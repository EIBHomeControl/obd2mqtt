# Changelog

## 0.3.6
- Fix: Absturzschleife durch volles Dateisystem – die Log-Dateien konnten zusammen ~96 KB von 128 KB belegen; jetzt max. 2 × 16 KB, und das Log belegt nie mehr als 70 % des Flash (Profile/Einstellungen haben Vorrang)
- Fix: Log-Dateien werden nur noch unter Sperre gelesen/geschrieben – Herunterladen konnte mit dem gleichzeitigen Schreiben/Rotieren kollidieren
- Selbstheilung: nach 2 Abstürzen in Folge wird das ältere Log gelöscht, nach 3 auch das aktuelle; ebenso beim Start, wenn das Dateisystem zu voll ist; alte 48-KB-Logs werden beim Update entfernt
- System: freier Heap mit Minimum und größtem Block sowie Belegung des Dateisystems
- Status im sicheren Modus zeigt MQTT-Verbindung wieder korrekt an
- Log-Zeilen direkt nach dem Start haben gleich Ortszeit statt UTC

## 0.3.5
- MQTT: aktives Fahrzeugprofil unter <base>/profile (+ Details als JSON unter <base>/profile/attributes), in Home Assistant als Sensor „Fahrzeugprofil“
- MQTT: alle Werte inkl. Profil und Zeitstempel als ein JSON unter <base>/state nach jedem Abfragezyklus

## 0.3.4
- Zeitzone als verständliche Auswahlliste (z. B. „Mitteleuropa – Deutschland, Österreich, Schweiz …“) statt POSIX-Angabe; eigene Angabe weiterhin möglich
- Aktuelle Uhrzeit am Gerät wird unter der Zeitzone angezeigt
- Dongle wird mit Namen angezeigt: Bluetooth-Name, erkannter Typ und ELM-Version (z. B. „vLinker MC · vLinker / iOS-Vlink · ELM327 v2.2“), auch im Status und im Log

## 0.3.3
- Profilauswahl zeigt sprechende Namen statt Dateinamen (z. B. „Hyundai IONIQ 5 / Kia EV6“, „XPeng G9“), sortiert, aktives Profil mit ✓
- „Kopie speichern“ fragt nur noch nach einem Namen – der interne Dateiname wird automatisch erzeugt
- Werksprofile umbenannt: „Hyundai IONIQ 5 / Kia EV6“, „XPeng P7+ (experimentell)“

## 0.3.2
- Werksprofile: alle Werte standardmäßig aktiviert (IONIQ 5, G9, G6, P7+)
- Profil-Tabelle: Häkchen im Spaltenkopf „Aktiv“ schaltet alle Werte an/aus

## 0.3.1
- Neue Werksprofile: XPeng G6 und XPeng P7+ (P7+ experimentell)
- Bestehende Geräte erhalten die neuen Profile automatisch nach dem Update

## 0.3.0
- Komplette Oberfläche, Gerätemeldungen und Handbuch auf Deutsch und Englisch – Sprachwahl oben rechts
- Sprache wird im Gerät gespeichert; Log, Fehlermeldungen und Home-Assistant-Namen folgen der Auswahl
- Werksprofile mit deutschen bzw. englischen Wertenamen
- Changelog zweisprachig (CHANGELOG.md / CHANGELOG.en.md)

## 0.2.5
- Einstellungen und Profile sichern (JSON-Datei, wahlweise inkl. Passwörter)
- Sicherung wiederherstellen – Einstellungen und/oder Profile, danach automatischer Neustart

## 0.2.4
- Benutzerhandbuch direkt im Gerät (Link „Hilfe“ oben rechts bzw. /hilfe)
- Handbuch mit Suchfeld und Live-Prüfung des eigenen Systems mit Links zur passenden Hilfe

## 0.2.3
- Log mit echter Uhrzeit (NTP, Zeitzone einstellbar; vor der Synchronisation Laufzeit „+hh:mm:ss“)
- Log dauerhaft im Flash speichern (übersteht Neustarts, rotiert bei 48 KB), abschaltbar
- Log: Kopieren, Herunterladen, älteres Log herunterladen, Löschen
- Changelog im System-Tab

## 0.2.2
- Aufwärmphase nach Dongle-Reset (ATRV + Pause) – behebt CAN ERROR beim Test-Button
- Einmalige Wiederholung bei CAN ERROR
- BLE bleibt nach Test/Terminal 60 s verbunden

## 0.2.1
- Verspätete ELM-Prompts („>“) werden ignoriert – behebt abgeschnittene Antworten
- Vor jedem Befehl warten, bis der Dongle still ist
- Bis zu 2 Wiederholungen bei unvollständiger Antwort, Rohantwort im Log

## 0.2.0
- Watchdog: Hänger > 60 s, WLAN 15 min bzw. MQTT 20 min weg → Neustart
- Sicherer Modus nach 3 Abstürzen in Folge (ohne BLE, Web-UI erreichbar)
- Grund des letzten Neustarts im System-Tab

## 0.1.10
- Fix: „Werksprofile wiederherstellen“ und „Profil löschen“ meldeten „Ungültiger Profilname“

## 0.1.9
- Unvollständige Multi-Frame-Antworten werden erkannt und wiederholt
- IONIQ 5: Init-Befehl ATST96 (längerer Timeout)
- Klarere Log-Meldung bei planmäßiger BLE-Trennung

## 0.1.8
- Status-LED entfernt (ESP32-DevKitC V4 hat keine steuerbare LED)

## 0.1.7
- Firmware-Update mit echtem Fortschritt, Fehlermeldungen und Warten auf Neustart
- Prüfung auf factory.bin, BLE-Abfragen pausieren während des Updates

## 0.1.5 – 0.1.6
- Versionsnummer im Dateinamen und in der Weboberfläche
- LED-Test (später entfernt)

## 0.1.4
- XPeng G9: SoH 22110A, Kilometerstand 3 Bytes, CLTC-Reichweite 221118, Ladelimit/Ladestatus

## 0.1.3
- Batterietemperatur, Kilometerstand, berechnete Reichweite; G9-Profil nach WiCAN

## 0.1.1
- WLAN-Suche in den Einstellungen

## 0.1.0
- Erste Version: BLE-ELM327 → MQTT mit Home-Assistant-Discovery, Weboberfläche, Profile, Test, Terminal, OTA
