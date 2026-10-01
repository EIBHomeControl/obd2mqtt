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
static uint32_t staConnectedSince = 0, staLostSince = 0;

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

static void wifiBegin() {
  WiFi.persistent(false);
  WiFi.setHostname(cfg.hostname.c_str());
  if (cfg.wifiSsid.isEmpty()) { startAP(); return; }
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPass.c_str());
  logf(T("Verbinde mit WLAN '%s'…", "Connecting to WiFi '%s'…"), cfg.wifiSsid.c_str());
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) delay(200);
  if (WiFi.status() == WL_CONNECTED) {
    logf(T("WLAN verbunden, IP %s", "WiFi connected, IP %s"), WiFi.localIP().toString().c_str());
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
  if (up) {
    staLostSince = 0;
    if (!staConnectedSince) {
      staConnectedSince = now;
      logf(T("WLAN verbunden, IP %s", "WiFi connected, IP %s"), WiFi.localIP().toString().c_str());
    }
    // AP abschalten, wenn STA stabil läuft und niemand am AP hängt
    if (apActive && now - staConnectedSince > 120000 && WiFi.softAPgetStationNum() == 0) stopAP();
  } else {
    staConnectedSince = 0;
    if (!staLostSince) staLostSince = now;
    if (!apActive && now - staLostSince > 180000) startAP();   // 3 min ohne WLAN → Setup-AP
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  logf("OBD2MQTT %s start", FW_VERSION);
  if (!fsBegin()) logf("LittleFS error!");
  bool cfgOk = loadConfig();
  Watchdog::begin();
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
  wifiLoop();
  Watchdog::loop(cfg.wifiSsid.isEmpty() || WiFi.status() == WL_CONNECTED, cfg.mqttHost.length() > 0, MqttHa::connected(),
                 apActive && WiFi.softAPgetStationNum() > 0);
  MqttHa::loop();
  Poller::loop();
  Web::loop();

  static bool timeLogged = false;
  if (!timeLogged && timeValid()) {
    timeLogged = true;
    logf(T("Uhrzeit per NTP synchronisiert (%s)", "Time synchronized via NTP (%s)"), cfg.ntpServer.c_str());
  }
  static uint32_t lastFlush = 0;
  if (millis() - lastFlush > 5000) { lastFlush = millis(); logFlush(); }
  delay(10);
}
