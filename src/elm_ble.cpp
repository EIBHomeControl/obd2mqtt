#include "elm_ble.h"
#include <NimBLEDevice.h>
#include <mutex>
#include <map>
#include "config.h"
#include "log.h"
#include "watchdog.h"
#include "i18n.h"

namespace ElmBle {

volatile bool quiet = false;


static NimBLEClient* client = nullptr;
static NimBLERemoteCharacteristic* chrNotify = nullptr;
static NimBLERemoteCharacteristic* chrWrite = nullptr;
static bool writeWithResponse = false;
static String uuidInfo, devName, nameMac, kind;
static volatile uint32_t connectMs = 0;    // Zeitpunkt des letzten erfolgreichen Verbindens

static std::mutex rxMutex;
static String rxBuf;
static volatile bool rxPrompt = false;
static bool rxContent = false;          // schon echte Antwortzeichen empfangen?
static volatile uint32_t rxLastMs = 0;
static volatile uint32_t strayPrompts = 0;
static volatile uint32_t drops = 0;        // ungeplante Verbindungsabbrüche (nicht 0x216)
static volatile uint8_t fastDrops = 0;     // Abbrüche 0x208 mit kurzem Verbindungsintervall
static volatile bool relaxed = false;      // true = langsamere Standard-Verbindungsparameter
static bool pairLogged = false;            // „Kopplung OK“ nur beim ersten Mal bzw. nach einem Fehler

static void onNotify(NimBLERemoteCharacteristic*, uint8_t* data, size_t len, bool) {
  std::lock_guard<std::mutex> lk(rxMutex);
  rxLastMs = millis();
  for (size_t i = 0; i < len; i++) {
    char c = (char)data[i];
    if (c == 0) continue;
    if (c == '>') {
      // Ein '>' vor jeder Antwort ist ein verspäteter Prompt des vorherigen Befehls
      // (z.B. doppelter Prompt nach ATZ) – ignorieren, sonst wird die Antwort abgeschnitten.
      if (!rxContent) { strayPrompts++; continue; }
      rxPrompt = true;
      continue;
    }
    if (rxPrompt) continue;              // nach dem Prompt nichts mehr anhängen
    if (c != '\r' && c != '\n' && c != ' ') rxContent = true;
    if (rxBuf.length() < 4096) rxBuf += c;
  }
}

// Wartet, bis der Dongle still ist (keine Notifications mehr für quietMs)
static void drain(uint32_t quietMs, uint32_t maxMs) {
  uint32_t t0 = millis();
  while (millis() - t0 < maxMs && millis() - rxLastMs < quietMs) delay(5);
}

class ClientCb : public NimBLEClientCallbacks {
  // Kopplung mit PIN (z. B. WiCAN Pro: Passkey-Eingabe, verschlüsselte Verbindung)
  void onPassKeyEntry(NimBLEConnInfo& info) override {
    if (cfg.blePin.isEmpty()) {
      logf("%s", T("Dongle verlangt eine PIN – bitte unter Einstellungen → BLE-Dongle eintragen", "Dongle requires a PIN – please enter it under Settings → BLE dongle"));
      NimBLEDevice::injectPassKey(info, 0);
      return;
    }
    logf("%s", T("Dongle verlangt PIN – sende gespeicherte PIN", "Dongle requires PIN – sending stored PIN"));
    NimBLEDevice::injectPassKey(info, (uint32_t)cfg.blePin.toInt());
  }
  void onConfirmPasskey(NimBLEConnInfo& info, uint32_t) override { NimBLEDevice::injectConfirmPasskey(info, true); }
  void onAuthenticationComplete(NimBLEConnInfo& info) override {
    if (info.isEncrypted()) {
      if (!pairLogged) logf("%s", T("BLE-Kopplung OK (verschlüsselt)", "BLE pairing OK (encrypted)"));
      pairLogged = true;
    } else {
      pairLogged = false;
      logf("%s", T("BLE-Kopplung fehlgeschlagen – PIN prüfen (gespeicherte Kopplung wird verworfen)", "BLE pairing failed – check the PIN (stored bond is discarded)"));
      NimBLEDevice::deleteBond(info.getIdAddress());
    }
  }
  void onDisconnect(NimBLEClient*, int reason) override {
    if (reason == 0x216) {   // 0x216 = von uns getrennt (planmäßig)
      if (!quiet) logf("%s", T("BLE getrennt (planmäßig – Dongle darf bis zur nächsten Abfrage schlafen)", "BLE disconnected (scheduled – dongle may sleep until the next poll)"));
      return;
    }
    drops++;
    logf(T("BLE getrennt (Grund 0x%X)", "BLE disconnected (reason 0x%X)"), reason);
    // 0x208 = Verbindung abgerissen (Supervision Timeout). Kommt das mit dem kurzen Verbindungsintervall
    // wiederholt vor, verträgt der Dongle es vermutlich nicht → ab jetzt Standardwerte (30–50 ms).
    // Nur Abbrüche kurz nach dem Verbinden zählen (Auto wegfahren o. ä. soll nicht umschalten)
    if (reason == 0x208 && !relaxed && millis() - connectMs < 60000 && ++fastDrops >= 2) {
      relaxed = true;
      logf("%s", T("Verbindung riss wiederholt ab (0x208) – nutze ab jetzt langsamere BLE-Verbindungsparameter",
                   "Connection dropped repeatedly (0x208) – using slower BLE connection parameters from now on"));
    }
  }
};
static ClientCb clientCb;

void begin() {
  NimBLEDevice::init("obd2mqtt");
  NimBLEDevice::setMTU(247);   // größere Pakete: lange Antworten (Multi-Frame) in weniger Notifications
  NimBLEDevice::setPower(9);   // max. TX-Leistung (dBm), hilft bei Garage → Auto
}

bool connected() { return client && client->isConnected(); }
uint32_t dropCount() { return drops; }
bool relaxedParams() { return relaxed; }
// RSSI direkt nach dem Verbinden ist oft noch nicht gemessen (z. B. -128) → 0 = unbekannt
int rssi() {
  if (!connected()) return 0;
  int r = client->getRssi();
  return (r <= -120 || r >= 0) ? 0 : r;
}
uint16_t mtu() { return connected() ? client->getMTU() : 0; }
String detectedUuids() { return uuidInfo; }
String deviceName() { return devName.length() && nameMac == cfg.bleMac ? devName : cfg.bleName; }
String dongleKind() { return kind; }

void disconnect() {
  if (client) {
    if (client->isConnected()) client->disconnect();
    NimBLEDevice::deleteClient(client);
  }
  client = nullptr;
  chrNotify = chrWrite = nullptr;
}

static bool isStdService(const NimBLEUUID& u) {
  static const uint16_t std16[] = {0x1800, 0x1801, 0x180A, 0x180F, 0xFE59};
  for (uint16_t s : std16) if (u == NimBLEUUID(s)) return true;
  return false;
}

// Sucht Notify- und Write-Characteristic. Reihenfolge: manuelle UUIDs → bekannte Paare → generisch.
static bool resolveCharacteristics(String& err) {
  chrNotify = chrWrite = nullptr;

  if (cfg.bleService.length()) {
    NimBLERemoteService* s = client->getService(NimBLEUUID(cfg.bleService.c_str()));
    if (!s) { err = "Service " + cfg.bleService + T(" nicht gefunden", " not found"); return false; }
    if (cfg.bleNotify.length()) chrNotify = s->getCharacteristic(NimBLEUUID(cfg.bleNotify.c_str()));
    if (cfg.bleWrite.length()) chrWrite = s->getCharacteristic(NimBLEUUID(cfg.bleWrite.c_str()));
    if (chrNotify && chrWrite) return true;
  }

  struct Known { const char *svc, *ntf, *wr; };
  static const Known known[] = {
    {"fff0", "fff1", "fff2"},   // viele Clone, Vgate iCar Pro u.a.
    {"ffe0", "ffe1", "ffe1"},   // HM-10-basierte Dongles
    {"18f0", "2af0", "2af1"},   // Vgate / Viecar
    {"e7810a71-73ae-499d-8c15-faa9aef0c3f2", "bef8d6c9-9c21-4c9e-b632-bd58c1009f9f",
     "bef8d6c9-9c21-4c9e-b632-bd58c1009f9f"},   // vLinker / iOS-Vlink u.a.
  };
  for (auto& k : known) {
    Watchdog::feed();   // jede Dienstsuche kann bis zu 30 s dauern
    if (!client->isConnected()) { err = T("Verbindung beim Aufbau abgerissen", "Connection dropped during setup"); return false; }
    NimBLERemoteService* s = client->getService(NimBLEUUID(k.svc));
    if (!s) continue;
    auto* n = s->getCharacteristic(NimBLEUUID(k.ntf));
    auto* w = s->getCharacteristic(NimBLEUUID(k.wr));
    if (n && w && (n->canNotify() || n->canIndicate()) && (w->canWrite() || w->canWriteNoResponse())) {
      chrNotify = n; chrWrite = w;
      return true;
    }
  }

  // Generisch: erster Nicht-Standard-Service mit Notify + Write
  Watchdog::feed();
  if (!client->isConnected()) { err = T("Verbindung beim Aufbau abgerissen", "Connection dropped during setup"); return false; }
  for (NimBLERemoteService* s : client->getServices(true)) {
    Watchdog::feed();
    if (isStdService(s->getUUID())) continue;
    NimBLERemoteCharacteristic *n = nullptr, *w = nullptr;
    for (NimBLERemoteCharacteristic* c : s->getCharacteristics(true)) {
      if (!n && (c->canNotify() || c->canIndicate())) n = c;
      if (!w && (c->canWrite() || c->canWriteNoResponse())) w = c;
    }
    if (n && w) { chrNotify = n; chrWrite = w; return true; }
  }
  err = T("Keine passende Notify/Write-Characteristic gefunden – UUIDs manuell eintragen", "No suitable notify/write characteristic found – enter UUIDs manually");
  return false;
}

bool connect(String& err) {
  if (connected()) return true;
  if (cfg.bleMac.length() != 17) { err = T("Keine gültige BLE-MAC konfiguriert", "No valid BLE MAC configured"); return false; }
  disconnect();

  // Sicherheit: mit PIN → Passkey-Eingabe (MITM, Secure Connections, Bonding); ohne PIN → "Just Works"
  if (cfg.blePin.length()) {
    NimBLEDevice::setSecurityAuth(true, true, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY);
  } else {
    NimBLEDevice::setSecurityAuth(true, false, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  }
  client = NimBLEDevice::createClient();
  client->setClientCallbacks(&clientCb, false);
  client->setConnectTimeout(6000);
  // Kurzes Verbindungsintervall (7,5–15 ms statt Standard 30–50 ms): mehr Durchsatz, damit der Puffer
  // billiger Dongles bei langen Antworten nicht überläuft (Ursache für „Unvollständige Antwort“).
  // Manche Dongles (z. B. Vgate) brechen damit ab → nach wiederholtem 0x208 Standardwerte.
  if (relaxed) client->setConnectionParams(24, 40, 0, 600);
  else client->setConnectionParams(6, 12, 0, 400);

  std::vector<uint8_t> types;
  if (cfg.bleAddrType == "public") types = {BLE_ADDR_PUBLIC};
  else if (cfg.bleAddrType == "random") types = {BLE_ADDR_RANDOM};
  else types = {BLE_ADDR_PUBLIC, BLE_ADDR_RANDOM};

  // Jeder Schritt blockiert bis zu 30 s (BLE-Timeout). Damit der 60-s-Watchdog nicht zuschlägt:
  // Watchdog vor jedem Schritt füttern, Schritt für die Absturzanalyse merken, nach 20 s abbrechen.
  const uint32_t t0 = millis();
  auto step = [&](const char* name) -> bool {
    Watchdog::feed();
    Watchdog::step(name);
    if (millis() - t0 > 20000) {
      err = String(T("Verbindungsaufbau dauert zu lange (abgebrochen bei: ", "Connection setup takes too long (aborted at: ")) + name + ")";
      return false;
    }
    if (strcmp(name, "BLE connect") && !client->isConnected()) {
      err = T("Verbindung beim Aufbau abgerissen", "Connection dropped during setup");
      return false;
    }
    return true;
  };
  auto fail = [&]() { Watchdog::step("BLE disconnect"); disconnect(); Watchdog::step("Poller"); return false; };

  bool ok = false;
  for (uint8_t t : types) {
    if (!step("BLE connect")) return fail();
    NimBLEAddress addr(std::string(cfg.bleMac.c_str()), t);
    if (client->connect(addr)) { ok = true; break; }
  }
  if (!ok) { err = T("Dongle nicht erreichbar (Auto weg / Dongle schläft?)", "Dongle not reachable (car away / dongle asleep?)"); return fail(); }

  if (!step("BLE MTU")) return fail();
  client->exchangeMTU();
  if (!step("BLE services")) return fail();
  if (!resolveCharacteristics(err)) return fail();

  if (!step(cfg.blePin.length() ? "BLE subscribe/pairing" : "BLE subscribe")) return fail();
  bool sub = chrNotify->canNotify() ? chrNotify->subscribe(true, onNotify)
                                    : chrNotify->subscribe(false, onNotify);
  if (!sub) {
    if (err.isEmpty()) err = T("Subscribe fehlgeschlagen", "Subscribe failed");
    return fail();
  }
  writeWithResponse = !chrWrite->canWriteNoResponse();

  uuidInfo = String(chrNotify->getRemoteService()->getUUID().toString().c_str()) + " / " +
             chrNotify->getUUID().toString().c_str() + " / " + chrWrite->getUUID().toString().c_str();
  // Gerätenamen aus dem GAP-Service lesen (0x1800 / 0x2A00) – nicht jeder Dongle stellt ihn bereit.
  // Nur beim ersten Verbinden: spart bei jedem Neuaufbau eine weitere (blockierende) Leseanfrage.
  if ((devName.isEmpty() || nameMac != cfg.bleMac) && step("BLE name")) {
    NimBLEAttValue v = client->getValue(NimBLEUUID((uint16_t)0x1800), NimBLEUUID((uint16_t)0x2A00));
    String n;
    for (size_t i = 0; i < v.length() && i < 40; i++) {
      char c = (char)v.data()[i];
      if (c >= 32 && c < 127) n += c;
    }
    n.trim();
    if (n.length()) { devName = n; nameMac = cfg.bleMac; }
  }
  // Typ grob anhand der Service-UUID
  {
    String svc = chrNotify->getRemoteService()->getUUID().toString().c_str();
    svc.toLowerCase();
    if (svc.indexOf("fff0") >= 0) kind = "ELM327 (FFF0)";
    else if (svc.indexOf("ffe0") >= 0) kind = "ELM327 / HM-10 (FFE0)";
    else if (svc.indexOf("18f0") >= 0) kind = "Vgate / Viecar (18F0)";
    else if (svc.indexOf("e7810a71") >= 0) kind = "vLinker / iOS-Vlink";
    else kind = T("unbekannter Typ", "unknown type");
  }
  Watchdog::step("Poller");
  connectMs = millis();
  if (!client->isConnected()) { err = T("Verbindung beim Aufbau abgerissen", "Connection dropped during setup"); return fail(); }
  if (!quiet) {
    int r = rssi();
    String rs = r ? String(r) + " dBm" : String(T("noch nicht gemessen", "not measured yet"));
    logf(T("BLE verbunden: %s (%s), RSSI %s, MTU %u, UUIDs %s%s", "BLE connected: %s (%s), RSSI %s, MTU %u, UUIDs %s%s"),
         deviceName().length() ? deviceName().c_str() : cfg.bleMac.c_str(), kind.c_str(), rs.c_str(),
         (unsigned)client->getMTU(), uuidInfo.c_str(),
         relaxed ? T(", langsame Verbindungsparameter", ", slow connection parameters") : "");
  }
  return true;
}

bool command(const String& cmd, String& response, uint32_t timeoutMs) {
  response = "";
  if (!connected() || !chrWrite) return false;
  drain(30, 300);                        // Reste der vorherigen Antwort abwarten
  {
    std::lock_guard<std::mutex> lk(rxMutex);
    rxBuf = "";
    rxPrompt = false;
    rxContent = false;
  }
  String out = cmd + "\r";
  const uint8_t* p = (const uint8_t*)out.c_str();
  size_t left = out.length();
  while (left) {   // in 20-Byte-Stücke (Default-MTU) aufteilen
    size_t n = min(left, (size_t)20);
    if (!chrWrite->writeValue(p, n, writeWithResponse)) return false;
    p += n; left -= n;
  }
  uint32_t t0 = millis();
  while (millis() - t0 < timeoutMs) {
    if (rxPrompt) break;
    if (!connected()) return false;
    Watchdog::feed();
    delay(5);
  }
  std::lock_guard<std::mutex> lk(rxMutex);
  response = rxBuf;
  return rxPrompt;
}

// Merkt sich je Gerät den zuletzt gesehenen Namen: Mit abgeschaltetem Duplikat-Filter überschreibt eine
// spätere Werbung ohne Scan-Antwort sonst den Namen wieder.
static std::mutex scanMutex;
static std::map<std::string, std::string> scanNames;
class ScanCb : public NimBLEScanCallbacks {
  void remember(const NimBLEAdvertisedDevice* d) {
    std::string n = d->getName();
    if (n.empty()) return;
    std::lock_guard<std::mutex> lk(scanMutex);
    scanNames[d->getAddress().toString()] = n;
  }
  void onDiscovered(const NimBLEAdvertisedDevice* d) override { remember(d); }
  void onResult(const NimBLEAdvertisedDevice* d) override { remember(d); }
};
static ScanCb scanCb;

std::vector<ScanEntry> scan(uint32_t ms) {
  std::vector<ScanEntry> out;
  NimBLEScan* sc = NimBLEDevice::getScan();
  {
    std::lock_guard<std::mutex> lk(scanMutex);
    scanNames.clear();
  }
  sc->setScanCallbacks(&scanCb, true);
  sc->setActiveScan(true);
  sc->setInterval(100);
  sc->setWindow(99);
  // Ohne Duplikat-Filter: Der Controller merkt sich bereits gesehene Geräte auch über mehrere Scans
  // hinweg und verwirft dann die Scan-Antwort – in der steht bei vielen Dongles der Name.
  // Folge war: beim zweiten Suchen nur noch die MAC ohne Namen.
  sc->setDuplicateFilter(0);   // (setScanCallbacks(..., true) setzt das ebenfalls)
  NimBLEScanResults res = sc->getResults(ms, false);
  for (int i = 0; i < res.getCount(); i++) {
    const NimBLEAdvertisedDevice* d = res.getDevice(i);
    ScanEntry e;
    e.mac = d->getAddress().toString().c_str();
    e.mac.toUpperCase();
    e.name = d->getName().c_str();
    if (e.name.isEmpty()) {
      std::lock_guard<std::mutex> lk(scanMutex);
      auto it = scanNames.find(d->getAddress().toString());
      if (it != scanNames.end()) e.name = it->second.c_str();
    }
    // Name trotzdem leer (z. B. Dongle sendet ihn nur selten): beim eingestellten Dongle den bekannten Namen nehmen
    if (e.name.isEmpty() && e.mac.equalsIgnoreCase(cfg.bleMac)) e.name = deviceName();
    e.rssi = d->getRSSI();
    e.addrRandom = d->getAddress().getType() == BLE_ADDR_RANDOM;
    String n = e.name; n.toUpperCase();
    e.likelyObd = n.indexOf("OBD") >= 0 || n.indexOf("ELM") >= 0 || n.indexOf("VLINK") >= 0 ||
                  n.indexOf("VGATE") >= 0 || n.indexOf("IOS") >= 0 || n.indexOf("CX") >= 0 ||
                  n.indexOf("VEEPEAK") >= 0 || n.indexOf("KIWI") >= 0 ||
                  d->isAdvertisingService(NimBLEUUID((uint16_t)0xFFF0)) ||
                  d->isAdvertisingService(NimBLEUUID((uint16_t)0xFFE0)) ||
                  d->isAdvertisingService(NimBLEUUID((uint16_t)0x18F0));
    out.push_back(e);
  }
  sc->clearResults();
  std::sort(out.begin(), out.end(), [](const ScanEntry& a, const ScanEntry& b) {
    if (a.likelyObd != b.likelyObd) return a.likelyObd;
    return a.rssi > b.rssi;
  });
  return out;
}

}  // namespace ElmBle
