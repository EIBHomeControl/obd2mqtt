#include "elm_ble.h"
#include <NimBLEDevice.h>
#include <mutex>
#include "config.h"
#include "log.h"
#include "watchdog.h"
#include "i18n.h"

namespace ElmBle {

static NimBLEClient* client = nullptr;
static NimBLERemoteCharacteristic* chrNotify = nullptr;
static NimBLERemoteCharacteristic* chrWrite = nullptr;
static bool writeWithResponse = false;
static String uuidInfo, devName, kind;

static std::mutex rxMutex;
static String rxBuf;
static volatile bool rxPrompt = false;
static bool rxContent = false;          // schon echte Antwortzeichen empfangen?
static volatile uint32_t rxLastMs = 0;
static volatile uint32_t strayPrompts = 0;

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
  void onDisconnect(NimBLEClient*, int reason) override {
    if (reason == 0x216) logf("%s", T("BLE getrennt (planmäßig – Dongle darf bis zur nächsten Abfrage schlafen)", "BLE disconnected (scheduled – dongle may sleep until the next poll)"));
    else logf(T("BLE getrennt (Grund 0x%X)", "BLE disconnected (reason 0x%X)"), reason);
  }
};
static ClientCb clientCb;

void begin() {
  NimBLEDevice::init("obd2mqtt");
  NimBLEDevice::setPower(9);   // max. TX-Leistung (dBm), hilft bei Garage → Auto
}

bool connected() { return client && client->isConnected(); }
int rssi() { return connected() ? client->getRssi() : 0; }
String detectedUuids() { return uuidInfo; }
String deviceName() { return devName.length() ? devName : cfg.bleName; }
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
  for (NimBLERemoteService* s : client->getServices(true)) {
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

  client = NimBLEDevice::createClient();
  client->setClientCallbacks(&clientCb, false);
  client->setConnectTimeout(6000);

  std::vector<uint8_t> types;
  if (cfg.bleAddrType == "public") types = {BLE_ADDR_PUBLIC};
  else if (cfg.bleAddrType == "random") types = {BLE_ADDR_RANDOM};
  else types = {BLE_ADDR_PUBLIC, BLE_ADDR_RANDOM};

  bool ok = false;
  for (uint8_t t : types) {
    NimBLEAddress addr(std::string(cfg.bleMac.c_str()), t);
    if (client->connect(addr)) { ok = true; break; }
  }
  if (!ok) { err = T("Dongle nicht erreichbar (Auto weg / Dongle schläft?)", "Dongle not reachable (car away / dongle asleep?)"); disconnect(); return false; }

  if (!resolveCharacteristics(err)) { disconnect(); return false; }

  bool sub = chrNotify->canNotify() ? chrNotify->subscribe(true, onNotify)
                                    : chrNotify->subscribe(false, onNotify);
  if (!sub) { err = T("Subscribe fehlgeschlagen", "Subscribe failed"); disconnect(); return false; }
  writeWithResponse = !chrWrite->canWriteNoResponse();

  uuidInfo = String(chrNotify->getRemoteService()->getUUID().toString().c_str()) + " / " +
             chrNotify->getUUID().toString().c_str() + " / " + chrWrite->getUUID().toString().c_str();
  // Gerätenamen aus dem GAP-Service lesen (0x1800 / 0x2A00) – nicht jeder Dongle stellt ihn bereit
  {
    NimBLEAttValue v = client->getValue(NimBLEUUID((uint16_t)0x1800), NimBLEUUID((uint16_t)0x2A00));
    String n;
    for (size_t i = 0; i < v.length() && i < 40; i++) {
      char c = (char)v.data()[i];
      if (c >= 32 && c < 127) n += c;
    }
    n.trim();
    if (n.length()) devName = n;
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
  logf(T("BLE verbunden: %s (%s), RSSI %d dBm, UUIDs %s", "BLE connected: %s (%s), RSSI %d dBm, UUIDs %s"),
       deviceName().length() ? deviceName().c_str() : cfg.bleMac.c_str(), kind.c_str(), client->getRssi(), uuidInfo.c_str());
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

std::vector<ScanEntry> scan(uint32_t ms) {
  std::vector<ScanEntry> out;
  NimBLEScan* sc = NimBLEDevice::getScan();
  sc->setActiveScan(true);
  sc->setInterval(100);
  sc->setWindow(99);
  NimBLEScanResults res = sc->getResults(ms, false);
  for (int i = 0; i < res.getCount(); i++) {
    const NimBLEAdvertisedDevice* d = res.getDevice(i);
    ScanEntry e;
    e.mac = d->getAddress().toString().c_str();
    e.mac.toUpperCase();
    e.name = d->getName().c_str();
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
