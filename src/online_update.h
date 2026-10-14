#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

// Online-Update über GitHub-Releases (github.com/EIBHomeControl/obd2mqtt):
// prüft das neueste Release, zeigt dessen Änderungen und installiert auf Wunsch die *-esp32-ota.bin.
// Läuft im Haupt-Task (blockierend, Watchdog wird gefüttert) – Web/MQTT stoßen nur an.
namespace OnlineUpdate {
void loop();                         // aus loop(): Anfragen ausführen, tägliche Prüfung
void requestCheck();
bool requestInstall();               // false = keine neuere Version bekannt
void stateJson(JsonDocument& d);
void publishMqtt();                  // HA-„update“-Entität: installierte/neueste Version
}
