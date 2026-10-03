#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <mutex>

// Abfrage-Logik: BLE verbinden, Init, PIDs zyklisch abfragen, Werte publizieren.
// Läuft ausschließlich im Arduino-loop()-Task; die Web-UI kommuniziert über Jobs/Flags.
namespace Poller {

std::mutex& mutex();          // schützt cfg/profile/Status zwischen Web- und Loop-Task

void begin(bool bleEnabled = true);   // false = sicherer Modus (keine BLE-Abfragen)
void loop();

// Aufrufe aus dem Web-Task
void requestConfigApply(const String& json);  // übernimmt + speichert Konfiguration
void requestProfileReload();                  // aktives Profil neu laden
void requestPollNow();
void requestEnabled(bool on);                 // Hauptschalter (Web/MQTT) – wird im loop() gespeichert
void setPaused(bool p);                       // z.B. während OTA
bool isPaused();

enum JobType { JOB_RAW, JOB_TEST, JOB_SCAN, JOB_DIAG };
void setDebug(uint32_t minutes);
size_t diagChunk(size_t offset, uint8_t* buf, size_t maxLen);   // letzter Diagnose-Bericht (Text)              // Diagnose-Log für N Minuten (0 = aus)
bool submitJob(JobType t, const String& header, const String& cmd, const String& formula,
               float cap = 0, float cons = 0);   // cap/cons: Akku-Werte für den Test (0 = aktives Profil)
void jobJson(JsonDocument& d);                // {state: idle|pending|running|done, result: {...}}

void statusJson(JsonDocument& d);

}  // namespace Poller
