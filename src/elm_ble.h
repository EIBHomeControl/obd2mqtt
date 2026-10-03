#pragma once
#include <Arduino.h>
#include <vector>

// BLE-Client für ELM327-kompatible OBD2-Dongles
namespace ElmBle {

struct ScanEntry { String mac, name; int rssi; bool addrRandom; bool likelyObd; };

void begin();
bool connect(String& err);          // nutzt cfg.bleMac / UUIDs (oder Auto-Erkennung)
void disconnect();
extern volatile bool quiet;         // true = Verbinden/Trennen nicht loggen (Auto schläft, Prüfung jede Minute)
bool connected();
int  rssi();
uint16_t mtu();                     // ausgehandelte MTU (0 = nicht verbunden)
String detectedUuids();             // "svc / notify / write" nach erfolgreicher Verbindung
String deviceName();                // Bluetooth-Name des Dongles (GAP-Name bzw. aus dem Scan)
String dongleKind();                // grobe Typ-Erkennung anhand der UUIDs

// Sendet einen Befehl und wartet auf den ELM-Prompt '>'. Rückgabe: Rohantwort (ohne '>')
bool command(const String& cmd, String& response, uint32_t timeoutMs);

std::vector<ScanEntry> scan(uint32_t ms);

}  // namespace ElmBle
