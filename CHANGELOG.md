# Changelog

## 0.3.14
- Fix: Im Schlafmodus stand trotzdem jede Minute „BLE getrennt (Grund 0x216)“ im Log – füllte das Log in ca. 4 Stunden und verdrängte ältere Einträge

## 0.3.13
- Weniger Speicherbedarf: Log-Download und Diagnose-Bericht werden stückweise (je 1 KB) gesendet statt komplett im RAM aufgebaut – bisher kostete „Herunterladen/Kopieren“ kurzzeitig bis zu ~40 KB Heap (gemessenes Minimum 25 KB)
- Diagnose-Bericht wird über /api/diag abgerufen (nicht mehr im Job-JSON)

## 0.3.12
- Neu: Diagnose-Bericht (Status → Diagnose) – frische Verbindung, jede Init-Antwort (abgelehnte Befehle markiert), ATI/AT@1/ATRV/ATDPN, jede Abfrage des Profils mit Rohantwort, Antwortzeit, Bytes mit Index und berechneten Werten, Zusammenfassung; Kopieren/Herunterladen; ohne WLAN-, IP- und Passwortdaten (Dongle-MAC gekürzt)
- Neu: Diagnose-Log für 2 Stunden – jede Anfrage mit Rohantwort, Init-Antworten, Spannung/Protokoll/MTU bei jeder Verbindung; schaltet sich selbst ab
- Init-Befehle, die der Dongle ablehnt („?“), stehen jetzt immer im Log
- Handbuch: Anleitung „Diagnose-Bericht für Beta-Tester“

## 0.3.11
- XPeng G6/G9/P7+: Init-Sequenz korrigiert – ATFCSD300000 vor ATFCSM1 ergänzt (sonst lehnt der ELM327 die eigene Flow-Control ab und mehrteilige Antworten bleiben aus; Fehler aus dem WiCAN-Profil), dazu ATAL
- Gespeicherte Profile werden beim Start automatisch repariert (fehlendes ATFCSD ergänzt, 7DF-Abfragen bei festem Empfangsfilter ATCRA abgeschaltet) – Hinweis im Log
- XPeng-Werksprofile: „SoC (OBD2 Standard)“ entfernt – konnte wegen des Empfangsfilters auf 784 nie antworten
- Schlafmodus mit Hysterese wie bei WiCAN: schlafen unter der Schwelle, wach erst ab Schwelle + 0,1 V
- Abfrage im Schlaf (alle N min) nur noch über 11,9 V
- Neu: 12V-Warnung – Home-Assistant-Sensor „12V niedrig“ (<base>/battery_low) unter einstellbarer Schwelle (Standard 12,0 V)

## 0.3.10
- BLE: größere Pakete (MTU 247) und kürzeres Verbindungsintervall – mehr Durchsatz gegen „Unvollständige Antwort“ bei langen Antworten und schwachem Signal; MTU steht im Log
- Schlafmodus: die minütliche Spannungsprüfung schreibt nicht mehr jedes Mal „BLE verbunden/getrennt“ ins Log (weniger Flash-Schreibzugriffe)
- IONIQ-5-Werksprofil: „AC-Stecker“ entfernt – bleibt auch mit gestecktem Ladekabel 0; bestehende Profile: Wert abschalten oder löschen

## 0.3.9
- IONIQ-5-Werksprofil: „Laden aktiv BMS“ (220106) entfernt – das Bit ist auch ohne Ladekabel 1, sobald das Auto wach ist (12-V-Nachladung); bestehende Profile: Wert abschalten oder löschen
- Schlafmodus: Kann die 12-V-Spannung nicht gelesen werden, bleibt ein schlafendes Auto in Ruhe (bisher wurde dann abgefragt und das Auto geweckt)
- Log zeigt, wenn „Jetzt abfragen“ ein schlafendes Auto weckt

## 0.3.8
- Neu: Hauptschalter „Abfrage EIN/AUS“ – auf der Status-Seite und in Home Assistant als Schalter (<base>/polling, Befehl an <base>/polling/set). Bei AUS verbindet sich die Bridge nicht mehr mit dem Auto; Test, Terminal und „Jetzt abfragen“ funktionieren weiter. Die Einstellung bleibt nach einem Neustart erhalten
- Neu: Auto schlafen lassen – liegt die 12-V-Spannung unter 13,2 V (Auto aus, lädt nicht), werden keine Anfragen mehr ans Auto geschickt, nur noch die Spannung am Dongle gemessen. Bisher hat die Abfrage alle 2 min den IONIQ 5 immer wieder geweckt (12 V sprang jedes Mal auf 14,7 V)
- Einstellungen → Abfrage: Schwelle „Auto schläft unter“ und optional „Abfrage, während das Auto schläft“ (alle N min); „Jetzt abfragen“ fragt immer ab
- Status zeigt „Auto wach/schläft“ mit Spannung; Home Assistant: neuer Sensor „Auto wach“ (<base>/car_awake)
- Fix: Die 10-min-Pause „Keine gültige Antwort“ griff auch bei unvollständigen Antworten, obwohl das Auto wach war – jetzt nur noch, wenn das Auto gar nicht antwortet
- IONIQ-5-Werksprofil: längeres Antwort-Timeout (ATSTFF) gegen unvollständige Mehrfach-Antworten (bestehende Profile: Init-Befehl ATST96 → ATSTFF ändern oder Werksprofile wiederherstellen)
- Weniger CAN-Verkehr: gleiche Anfragen werden pro Abfragezyklus nur noch einmal gesendet (bisher z. B. 220105 bis zu 3× pro Zyklus)
- IONIQ-5-Werksprofil: neue experimentelle Werte „Laden aktiv“, „DC-Laden“, „AC-Stecker“ (220101 Byte 12, laut WiCAN) und „Laden aktiv BMS“ (220106 Byte 27, laut evDash) – bitte mit Test prüfen

## 0.3.7
- Einstellungen → WLAN: IP-Adresse automatisch (DHCP) oder fest (IP, Subnetzmaske, Gateway, DNS); Knopf „Aktuelle Werte übernehmen“
- Fallback: Funktioniert die feste IP beim Start nicht (keine WLAN-Verbindung oder MQTT-Broker nicht erreichbar), holt sich die Bridge automatisch eine Adresse per DHCP – Hinweis im Log, unter System und in den Einstellungen
- Ungültige Angaben (z. B. Gateway nicht im Subnetz) werden schon beim Speichern bzw. beim Start erkannt

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
