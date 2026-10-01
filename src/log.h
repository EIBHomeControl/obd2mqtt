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
bool timeValid();
