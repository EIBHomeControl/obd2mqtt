#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

// Thread-sicherer Log: Ring-Puffer (RAM) + optional dauerhaft im Flash (/log.txt, rotiert nach /log.old.txt)
void logf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void logToJson(JsonArray arr, size_t maxLines = 60);
String logText();                 // kompletter RAM-Puffer als Text
void logSetPersist(bool on);
bool logPersist();
void logFlush();                  // ausstehende Zeilen in die Datei schreiben (aus loop())
void logClear();
bool logReadFile(bool old, String& out);
size_t logReadChunk(bool old, size_t offset, uint8_t* buf, size_t maxLen);   // stückweise lesen (wenig RAM)
bool logFileExists(bool old);   // Log-Datei lesen (thread-sicher)
void logBootCheck(uint32_t crashes);       // beim Start: Log-Dateien bei Absturzserie/vollem Flash entfernen
bool timeValid();                 // Uhrzeit per NTP gesetzt und plausibel
void timeInit(const char* tz);    // früh in setup(): Zeitzone, Plausibilitätsprüfung, NTP-Rückruf
void timeLoop(const char* server);  // in loop(): NTP-Antworten prüfen
