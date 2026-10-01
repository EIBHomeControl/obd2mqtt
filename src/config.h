#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>


// ---------- Geräte-Konfiguration (/config.json) ----------
struct AppConfig {
  // Netzwerk
  String wifiSsid, wifiPass;
  String hostname = "obd2mqtt";
  String webPass;                       // leer = keine Anmeldung (User: admin)

  // MQTT
  String mqttHost;
  uint16_t mqttPort = 1883;
  String mqttUser, mqttPass;
  String mqttBase = "obd2mqtt";         // State-Topics: <base>/<pid>
  String haPrefix = "homeassistant";    // Discovery-Prefix
  String deviceName = "Auto OBD";       // Gerätename in Home Assistant

  // BLE-Dongle
  String bleMac;                        // AA:BB:CC:DD:EE:FF
  String bleAddrType = "auto";          // auto | public | random
  String bleService, bleNotify, bleWrite; // leer = automatisch erkennen
  String bleName;                       // Anzeigename (aus Scan bzw. Dongle)

  // Abfrage
  String profile = "ioniq5";            // Datei /profiles/<name>.json
  float minVoltage = 12.2f;             // 12V-Schutz, 0 = aus
  uint32_t retrySec = 60;               // Pause nach fehlgeschlagener BLE-Verbindung
  uint32_t cmdTimeoutMs = 3000;
  bool keepConnected = false;           // BLE zwischen den Abfragen halten
  uint32_t backoffSec = 600;            // Pause, wenn das Auto nicht antwortet (0 = aus)

  // Zeit & Log
  String ntpServer = "pool.ntp.org";
  String tz = "CET-1CEST,M3.5.0,M10.5.0/3";  // POSIX-Zeitzone (Deutschland)
  bool logPersist = true;               // Log im Flash speichern (übersteht Neustarts)
  String lang = "de";                   // Sprache: de | en
};

// ---------- Fahrzeug-Profil (/profiles/<name>.json) ----------
struct PidDef {
  String id, name, header, cmd, formula;
  String unit, deviceClass, stateClass = "measurement", icon;
  uint32_t interval = 120;              // Sekunden
  float minVal = NAN, maxVal = NAN;     // Plausibilitätsfilter
  int precision = 1;
  bool enabled = true;
};

struct Profile {
  String name, model;
  std::vector<String> init;
  std::vector<PidDef> pids;
};

extern AppConfig cfg;
extern Profile profile;

bool fsBegin();
bool loadConfig();
bool saveConfig();
void configToJson(JsonDocument& doc, bool maskSecrets);
void configFromJson(JsonVariantConst src);   // "********" = Wert unverändert lassen

bool loadProfile(const String& name);
bool parseProfile(JsonVariantConst src, Profile& out, String& err);
void profileToJson(const Profile& p, JsonDocument& doc);
bool saveProfileRaw(const String& name, const String& json, String& err);
String readProfileRaw(const String& name);
std::vector<String> listProfiles();
void installDefaultProfiles(bool overwrite);
bool validProfileName(const String& n);

extern const char* MASK;
