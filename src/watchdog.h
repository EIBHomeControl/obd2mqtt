#pragma once
#include <Arduino.h>

// Watchdog & sicherer Modus
//  - Hardware-Task-Watchdog: hängt die Hauptschleife > 60 s → automatischer Neustart
//  - Verbindungs-Watchdog: WLAN bzw. MQTT länger als X min weg → Neustart
//  - Sicherer Modus: nach 3 Abstürzen/Watchdog-Resets in Folge startet die Firmware
//    ohne Bluetooth/Abfragen, damit die Weboberfläche erreichbar bleibt
namespace Watchdog {
void begin();                       // früh in setup() aufrufen
void armTaskWdt();                  // am Ende von setup()
void feed();                        // in langen Warteschleifen aufrufen
void loop(bool wifiUp, bool mqttConfigured, bool mqttUp, bool apClients);
bool safeMode();
uint32_t crashCount();
const char* resetReason();          // Grund des letzten Neustarts (Klartext)
void restart(const char* why);      // geplanter Neustart mit Begründung
void clearCrashCounter();           // z.B. bei manuellem Neustart
// Merkt sich (überlebt den Neustart), was die Firmware gerade tut. Hängt sie, steht nach dem
// Watchdog-Neustart im Log, wobei („Hing bei: BLE subscribe/pairing“). nullptr = nichts Besonderes.
void step(const char* what);
const char* hangStep();             // Schritt, bei dem die Firmware vor dem letzten Absturz hing ("" = unbekannt)
}
