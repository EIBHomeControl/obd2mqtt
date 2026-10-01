#include "mqtt_ha.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "log.h"
#include "i18n.h"

namespace MqttHa {

static WiFiClient net;
static PubSubClient mqtt(net);
static uint32_t lastTry = 0;
static bool discoveryPending = true;
static std::vector<String> announced;   // bereits per Discovery angelegte Sensor-IDs

String deviceId() {
  uint8_t m[6];
  WiFi.macAddress(m);
  char b[20];
  snprintf(b, sizeof(b), "obd2mqtt_%02x%02x%02x", m[3], m[4], m[5]);
  return b;
}

static String topic(const String& sub) { return cfg.mqttBase + "/" + sub; }

static void addDevice(JsonDocument& d) {
  JsonObject dev = d["device"].to<JsonObject>();
  dev["identifiers"][0] = deviceId();
  dev["name"] = cfg.deviceName;
  dev["model"] = profile.model.length() ? profile.model : String("OBD2 BLE Bridge");
  dev["manufacturer"] = "OBD2MQTT (ESP32)";
  dev["sw_version"] = FW_VERSION;
  dev["configuration_url"] = "http://" + WiFi.localIP().toString() + "/";
  JsonArray av = d["availability"].to<JsonArray>();
  av.add<JsonObject>()["topic"] = topic("status");
}

static void publishConfig(const String& component, const String& objId, JsonDocument& d) {
  String t = cfg.haPrefix + "/" + component + "/" + deviceId() + "/" + objId + "/config";
  String payload;
  if (!d.isNull()) {
    d["unique_id"] = deviceId() + "_" + objId;
    addDevice(d);
    serializeJson(d, payload);
  }
  mqtt.publish(t.c_str(), payload.c_str(), true);   // leerer Payload = Entität entfernen
}

// <base>/profile = Anzeigename, <base>/profile/attributes = Details (JSON)
void publishProfile() {
  if (!mqtt.connected()) return;
  mqtt.publish(topic("profile").c_str(), (profile.name.length() ? profile.name : cfg.profile).c_str(), true);
  JsonDocument a;
  a["id"] = cfg.profile;
  a["name"] = profile.name;
  a["model"] = profile.model;
  JsonArray en = a["active_pids"].to<JsonArray>();
  for (auto& p : profile.pids) if (p.enabled) en.add(p.id);
  a["firmware"] = FW_VERSION;
  String s;
  serializeJson(a, s);
  mqtt.publish(topic("profile/attributes").c_str(), s.c_str(), true);
}

static void sendDiscovery() {
  std::vector<String> now;
  for (auto& p : profile.pids) {
    JsonDocument d;
    if (p.enabled) {
      d["name"] = p.name;
      d["state_topic"] = topic(p.id);
      if (p.unit.length()) d["unit_of_measurement"] = p.unit;
      if (p.deviceClass.length()) d["device_class"] = p.deviceClass;
      if (p.stateClass.length()) d["state_class"] = p.stateClass;
      if (p.icon.length()) d["icon"] = p.icon;
      d["suggested_display_precision"] = p.precision;
      now.push_back(p.id);
    }
    publishConfig("sensor", p.id, d);
  }
  // Entfernte PIDs aus HA löschen
  for (auto& old : announced) {
    bool still = false;
    for (auto& n : now) if (n == old) still = true;
    bool inProfile = false;
    for (auto& p : profile.pids) if (p.id == old) inProfile = true;
    if (!still && !inProfile) { JsonDocument empty; publishConfig("sensor", old, empty); }
  }
  announced = now;

  { // 12V-Bordnetz (vom Dongle gemessen)
    JsonDocument d;
    d["name"] = T("12V Bordnetz", "12V battery");
    d["state_topic"] = topic("voltage_12v");
    d["unit_of_measurement"] = "V";
    d["device_class"] = "voltage";
    d["state_class"] = "measurement";
    d["suggested_display_precision"] = 1;
    publishConfig("sensor", "voltage_12v", d);
  }
  { // Dongle erreichbar
    JsonDocument d;
    d["name"] = T("OBD-Dongle verbunden", "OBD dongle connected");
    d["state_topic"] = topic("car");
    d["payload_on"] = "online";
    d["payload_off"] = "offline";
    d["device_class"] = "connectivity";
    publishConfig("binary_sensor", "car", d);
  }
  { // BLE-Signal
    JsonDocument d;
    d["name"] = "BLE Signal";
    d["state_topic"] = topic("ble_rssi");
    d["unit_of_measurement"] = "dBm";
    d["device_class"] = "signal_strength";
    d["entity_category"] = "diagnostic";
    publishConfig("sensor", "ble_rssi", d);
  }
  { // Letzter Fehler
    JsonDocument d;
    d["name"] = T("Letzter Fehler", "Last error");
    d["state_topic"] = topic("last_error");
    d["entity_category"] = "diagnostic";
    d["icon"] = "mdi:alert-circle-outline";
    publishConfig("sensor", "last_error", d);
  }
  { // Aktives Fahrzeugprofil (Name als Wert, Details als Attribute)
    JsonDocument d;
    d["name"] = T("Fahrzeugprofil", "Vehicle profile");
    d["state_topic"] = topic("profile");
    d["json_attributes_topic"] = topic("profile/attributes");
    d["entity_category"] = "diagnostic";
    d["icon"] = "mdi:car-info";
    publishConfig("sensor", "profile", d);
  }
  publishProfile();
  logf(T("MQTT Discovery gesendet (%u PIDs aktiv)", "MQTT discovery sent (%u PIDs active)"), (unsigned)now.size());
}

void begin() {
  mqtt.setBufferSize(1536);
  mqtt.setKeepAlive(30);
}

bool connected() { return mqtt.connected(); }
void requestDiscovery() { discoveryPending = true; }

void loop() {
  if (cfg.mqttHost.isEmpty() || WiFi.status() != WL_CONNECTED) return;
  if (!mqtt.connected()) {
    if (millis() - lastTry < 10000 && lastTry) return;
    lastTry = millis();
    mqtt.setServer(cfg.mqttHost.c_str(), cfg.mqttPort);
    String will = topic("status");
    bool ok = mqtt.connect(deviceId().c_str(),
                           cfg.mqttUser.length() ? cfg.mqttUser.c_str() : nullptr,
                           cfg.mqttPass.length() ? cfg.mqttPass.c_str() : nullptr,
                           will.c_str(), 1, true, "offline");
    if (!ok) { logf(T("MQTT-Verbindung fehlgeschlagen (rc=%d)", "MQTT connection failed (rc=%d)"), mqtt.state()); return; }
    logf(T("MQTT verbunden mit %s:%u", "MQTT connected to %s:%u"), cfg.mqttHost.c_str(), cfg.mqttPort);
    mqtt.publish(will.c_str(), "online", true);
    discoveryPending = true;
  }
  if (discoveryPending) { discoveryPending = false; sendDiscovery(); }
  mqtt.loop();
}

void publishValue(const String& id, double v, int precision) {
  if (!mqtt.connected()) return;
  mqtt.publish(topic(id).c_str(), String(v, precision).c_str(), true);
}

void publishText(const String& sub, const String& v, bool retain) {
  if (!mqtt.connected()) return;
  mqtt.publish(topic(sub).c_str(), v.c_str(), retain);
}

}  // namespace MqttHa
