// OBD2MQTT – ESP32-Bridge: BLE-OBD2-Dongle (ELM327) → MQTT → Home Assistant
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include "config.h"
#include "log.h"
#include "i18n.h"
#include "mqtt_ha.h"
#include "poller.h"
#include "watchdog.h"
#include "web.h"

static DNSServer dns;
static bool apActive = false;
static uint32_t staConnectedSince = 0, staLostSince = 0, staRetryMs = 0;
static volatile int staDiscReason = -1;   // aus dem WLAN-Ereignis (anderer Task) → in wifiLoop loggen
static int staLastRssi = 0;
static uint32_t staReconnects = 0;

static const char* wifiReasonText(int r) {
  switch (r) {
    case 2:   return T("Anmeldung abgelaufen", "auth expired");
    case 3:   return T("vom Router abgemeldet", "deauthenticated by router");
    case 4:   return T("Zuordnung abgelaufen (Inaktivität)", "association expired (inactivity)");
    case 8:   return T("Gerät hat sich abgemeldet", "station left");
    case 15:  return T("Schlüsselaustausch Zeitüberschreitung (Passwort?)", "4-way handshake timeout (password?)");
    case 200: return T("Beacon-Timeout (Signal zu schwach / Funkstörung)", "beacon timeout (weak signal / interference)");
    case 201: return T("WLAN nicht gefunden", "network not found");
    case 202: return T("Anmeldung fehlgeschlagen (Passwort?)", "authentication failed (password?)");
    case 203: return T("Zuordnung fehlgeschlagen", "association failed");
    case 204: return T("Handshake-Timeout", "handshake timeout");
    case 205: return T("Verbindung fehlgeschlagen", "connection failed");
    default:  return T("sonstiger Grund", "other reason");
  }
}

static void startAP() {
  if (apActive) return;
  uint8_t m[6];
  WiFi.macAddress(m);
  char ssid[32];
  snprintf(ssid, sizeof(ssid), "OBD2MQTT-%02X%02X", m[4], m[5]);
  WiFi.mode(cfg.wifiSsid.length() ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAP(ssid, "obd2mqtt");
  dns.start(53, "*", WiFi.softAPIP());
  apActive = true;
  logf(T("Access Point '%s' (Passwort obd2mqtt) → http://%s", "Access point '%s' (password obd2mqtt) → http://%s"), ssid, WiFi.softAPIP().toString().c_str());
}

static void stopAP() {
  if (!apActive) return;
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  apActive = false;
  logf("%s", T("Access Point beendet", "Access point stopped"));
}

bool g_ipFallback = false;   // feste IP eingestellt, aber DHCP-Fallback aktiv

// Feste IP aus der Konfiguration anwenden; false = ungültig/abgeschaltet → DHCP
static bool applyStaticIp() {
  if (cfg.ipMode != "static") return false;
  IPAddress ip, gw, mask, d1, d2;
  if (!ip.fromString(cfg.ipAddr) || !gw.fromString(cfg.ipGw) || !mask.fromString(cfg.ipMask)) {
    logf("%s", T("Feste IP: Angaben unvollständig/ungültig – verwende DHCP", "Static IP: settings incomplete/invalid – using DHCP"));
    return false;
  }
  if (((uint32_t)ip & (uint32_t)mask) != ((uint32_t)gw & (uint32_t)mask)) {
    logf("%s", T("Feste IP: Gateway liegt nicht im Subnetz – verwende DHCP", "Static IP: gateway not in subnet – using DHCP"));
    return false;
  }
  if (!d1.fromString(cfg.ipDns1)) d1 = gw;
  if (!d2.fromString(cfg.ipDns2)) d2 = IPAddress((uint32_t)0);
  return WiFi.config(ip, gw, mask, d1, d2);
}

static bool waitConnected(uint32_t ms) {
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < ms) delay(200);
  return WiFi.status() == WL_CONNECTED;
}

// Mit fester IP ist "verbunden" nur die Funkverbindung. Prüfen, ob das Netz wirklich passt:
// MQTT-Broker per TCP erreichbar (inkl. DNS, falls Hostname). Ohne Broker: kein Test möglich → ok.
static bool networkWorks() {
  if (cfg.mqttHost.isEmpty()) return true;
  for (int i = 0; i < 3; i++) {
    WiFiClient c;
    if (c.connect(cfg.mqttHost.c_str(), cfg.mqttPort, 3000)) { c.stop(); return true; }
    delay(1000);
  }
  return false;
}

static void wifiBegin() {
  WiFi.persistent(false);
  WiFi.setHostname(cfg.hostname.c_str());
  if (cfg.wifiSsid.isEmpty()) { startAP(); return; }
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) { staDiscReason = info.wifi_sta_disconnected.reason; },
               ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  bool useStatic = applyStaticIp();
  WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPass.c_str());
  logf(T("Verbinde mit WLAN '%s' (%s)…", "Connecting to WiFi '%s' (%s)…"), cfg.wifiSsid.c_str(),
       useStatic ? (String(T("feste IP ", "static IP ")) + cfg.ipAddr).c_str() : "DHCP");
  bool ok = waitConnected(20000);
  if (useStatic && (!ok || !networkWorks())) {
    logf(T("Feste IP %s funktioniert nicht (%s) – Fallback auf DHCP", "Static IP %s does not work (%s) – falling back to DHCP"),
         cfg.ipAddr.c_str(), ok ? T("MQTT-Broker nicht erreichbar", "MQTT broker not reachable") : T("keine WLAN-Verbindung", "no WiFi connection"));
    g_ipFallback = true;
    WiFi.disconnect(false, false);
    delay(200);
    WiFi.config(IPAddress((uint32_t)0), IPAddress((uint32_t)0), IPAddress((uint32_t)0));   // 0.0.0.0 = DHCP
    WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPass.c_str());
    ok = waitConnected(20000);
  }
  if (ok) {
    logf(T("WLAN verbunden, IP %s%s", "WiFi connected, IP %s%s"), WiFi.localIP().toString().c_str(),
         g_ipFallback ? T(" (DHCP-Fallback)", " (DHCP fallback)") : (useStatic ? T(" (fest)", " (static)") : " (DHCP)"));
    staConnectedSince = millis();
  } else {
    logf("%s", T("WLAN nicht erreichbar – starte Setup-AP", "WiFi not reachable – starting setup AP"));
    startAP();
  }
}

static void wifiLoop() {
  if (apActive) dns.processNextRequest();
  if (cfg.wifiSsid.isEmpty()) return;
  bool up = WiFi.status() == WL_CONNECTED;
  uint32_t now = millis();
  int dr = staDiscReason;
  if (dr >= 0) {
    staDiscReason = -1;
    if (staConnectedSince || !staLostSince)   // nur den ersten Abriss melden, nicht jeden gescheiterten Versuch
      logf(T("WLAN getrennt (Grund %d: %s, letztes Signal %d dBm)", "WiFi disconnected (reason %d: %s, last signal %d dBm)"),
           dr, wifiReasonText(dr), staLastRssi);
  }
  if (up) {
    staLostSince = 0;
    static uint32_t lastRssiMs = 0;
    if (now - lastRssiMs > 10000) { lastRssiMs = now; staLastRssi = WiFi.RSSI(); }
    if (!staConnectedSince) {
      staConnectedSince = now;
      logf(T("WLAN verbunden, IP %s, Signal %d dBm", "WiFi connected, IP %s, signal %d dBm"), WiFi.localIP().toString().c_str(), (int)WiFi.RSSI());
    }
    // AP abschalten, wenn STA stabil läuft und niemand am AP hängt
    if (apActive && now - staConnectedSince > 120000 && WiFi.softAPgetStationNum() == 0) stopAP();
  } else {
    staConnectedSince = 0;
    if (!staLostSince) { staLostSince = now; staRetryMs = now; }
    // Das automatische Wiederverbinden des ESP32-Cores bleibt bei manchen Trenngründen (z. B. FritzBox)
    // hängen → alle 30 s selbst neu verbinden
    if (now - staRetryMs > 30000) {
      staRetryMs = now;
      staReconnects++;
      WiFi.disconnect(false, false);
      delay(100);
      WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPass.c_str());
      if (staReconnects % 10 == 1)
        logf(T("WLAN: neuer Verbindungsversuch (seit %lus getrennt)", "WiFi: reconnect attempt (disconnected for %lus)"),
             (unsigned long)((now - staLostSince) / 1000));
    }
    if (!apActive && now - staLostSince > 180000) startAP();   // 3 min ohne WLAN → Setup-AP
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  bool fsOk = fsBegin();
  bool cfgOk = loadConfig();
  // Zeitzone sofort setzen: nach einem Software-Neustart läuft die Uhr weiter, Log-Zeilen sollen gleich Ortszeit haben
  timeInit(cfg.tz.c_str());
  logf("OBD2MQTT %s start", FW_VERSION);
  if (!fsOk) logf("LittleFS error!");
  Watchdog::begin();
  logBootCheck(Watchdog::crashCount());
  migrateProfiles();   // nach loadConfig, damit die Sprache für Auswahllisten stimmt
  if (!cfgOk) logf("%s", T("Keine Konfiguration – Standardwerte", "No configuration – using defaults"));
  logSetPersist(cfg.logPersist);
  configTzTime(cfg.tz.c_str(), cfg.ntpServer.c_str(), "time.cloudflare.com");

  wifiBegin();
  if (MDNS.begin(cfg.hostname.c_str())) MDNS.addService("http", "tcp", 80);

  MqttHa::begin();
  Poller::begin(!Watchdog::safeMode());
  Web::begin();
  Watchdog::armTaskWdt();
}

void loop() {
  Watchdog::step("WiFi");
  wifiLoop();
  Watchdog::loop(cfg.wifiSsid.isEmpty() || WiFi.status() == WL_CONNECTED, cfg.mqttHost.length() > 0, MqttHa::connected(),
                 apActive && WiFi.softAPgetStationNum() > 0);
  Watchdog::step("MQTT");
  MqttHa::loop();
  Watchdog::step("Poller");
  Poller::loop();
  Watchdog::step("Web");
  Web::loop();

  timeLoop(cfg.ntpServer.c_str());
  static uint32_t lastFlush = 0;
  if (millis() - lastFlush > 5000) { lastFlush = millis(); Watchdog::step("Log"); logFlush(); }
  Watchdog::step(nullptr);
  delay(10);
}
