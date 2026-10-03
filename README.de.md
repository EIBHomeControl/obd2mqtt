# OBD2MQTT – BLE-OBD2 → MQTT → Home Assistant

> **Beta.** Getestet am Hyundai IONIQ 5. Die XPeng-Profile (G6, G9, P7+) sind noch **ungetestet**, Rückmeldungen sind sehr willkommen (Issue oder Forum).
> 🇬🇧 *English version: [README.md](README.md)*

**[⬇ Download: neueste Firmware (Releases)](../../releases/latest)** · [Handbuch](docs/handbuch.html) · [Changelog](CHANGELOG.md)

ESP32-Firmware, die sich per Bluetooth LE mit einem ELM327-kompatiblen OBD2-Dongle im Auto verbindet, konfigurierbare PIDs abfragt und die Werte per MQTT (mit Home-Assistant-Auto-Discovery) veröffentlicht. Die gesamte Konfiguration läuft über eine Weboberfläche.

**Empfohlener Aufbau:** Der ESP32 hängt fest in der Garage (im WLAN, am USB-Netzteil), der Dongle steckt im Auto. Sobald das Auto in BLE-Reichweite steht (typisch 5–15 m), kommen die Werte an.

## Funktionen

- BLE-Client für ELM327-Dongles (Auto-Erkennung gängiger UUIDs: FFF0, FFE0, 18F0, vLinker; manuell überschreibbar)
- Fahrzeugprofile als JSON (vorinstalliert: Hyundai IONIQ 5 / Kia EV6, XPeng G6, G9 und P7+), in der Web-UI editierbar
- Weboberfläche und Handbuch auf Deutsch und Englisch, Sichern/Wiederherstellen der Einstellungen, Log mit Uhrzeit
- PIDs mit frei definierbarer Formel (`B34/2`, `u16(B28,B29)/10`, `s16(..)`, `s8(..)`, `bit(x,n)`)
- **Test-Button** pro PID: zeigt alle Antwort-Bytes mit Index an, damit du Byte-Positionen leicht findest
- ELM327-Terminal im Browser
- MQTT Auto-Discovery: Sensoren erscheinen automatisch als Gerät in Home Assistant
- 12-V-Schutz (ATRV), Pause wenn das Auto nicht antwortet, BLE wird zwischen den Abfragen getrennt, damit der Dongle schlafen kann
- Setup-Access-Point mit Captive Portal, mDNS (`http://obd2mqtt.local`), OTA-Update, optionaler Passwortschutz
- **Diagnose-Bericht** (ein Klick, kopieren/herunterladen, ohne WLAN-/IP-/Passwortdaten) und 2-Stunden-Diagnose-Log – ideal für Beta-Tester
- Dongles mit Kopplungs-PIN (z. B. WiCAN Pro: Bluetooth aktivieren, Protokoll „elm327“, PIN unter Einstellungen → BLE-Dongle)

## Flashen

**Variante A – ohne Toolchain (klassischer ESP32):**
Unter [Releases](../../releases/latest) die Datei `obd2mqtt-vX.Y.Z-esp32-factory.bin` herunterladen und mit einem Web-Flasher in Chrome/Edge (z. B. https://espressif.github.io/esptool-js/) an **Adresse 0x0** flashen.

**Variante B – PlatformIO (alle Boards):**
```bash
pio run -e esp32dev -t upload      # klassischer ESP32
pio run -e esp32s3 -t upload       # ESP32-S3
pio run -e esp32c3 -t upload       # ESP32-C3
pio device monitor
```

Spätere Updates: Web-UI → System → Firmware-Update mit `obd2mqtt-vX.Y.Z-esp32-ota.bin` aus den Releases (nach `pio run` liegt sie automatisch unter `firmware/obd2mqtt-vX.Y.Z-<env>-ota.bin`).

## Ersteinrichtung

1. Nach dem ersten Start öffnet der ESP32 das WLAN **`OBD2MQTT-xxxx`** (Passwort `obd2mqtt`). Verbinden und http://192.168.4.1 öffnen.
2. **Einstellungen → WLAN**: SSID und Passwort eintragen, danach **MQTT**: Broker, Benutzer und Passwort.
3. **BLE-Dongle**: Das Auto mit eingestecktem Dongle in die Nähe stellen, dann „Nach Dongles suchen“. Mit ★ markierte Einträge sind wahrscheinlich OBD-Adapter. Eintrag anklicken.
4. „Speichern & Neustart“. Danach ist die Oberfläche unter `http://obd2mqtt.local` oder der vergebenen IP erreichbar.
5. **Profil**: das passende Profil auswählen und „Aktivieren“. Mit „Test“ bei `soc` prüfen, ob ein Wert kommt.

## Profile

### Hyundai IONIQ 5 (auch Kia EV6 / IONIQ 6, E-GMP)

Init: `ATZ ATE0 ATL0 ATS0 ATH0 ATSP6 ATAT0 ATSTFF ATFCSH7E4 ATFCSD300000 ATFCSM1`. `ATAT0` (keine adaptive Wartezeit) und die feste Flow-Control machen lange Antworten (`220101`, 62 Bytes) mit billigen ELM327-Nachbauten zuverlässig – im Test mit einem „OBDII v1.5“-Dongle von 0 aus 3 auf 6 aus 7 vollständige Antworten. Die Flow-Control-Adresse wandert bei jedem Adresswechsel automatisch mit (`7E4`, `7C6`).

| ID | Header | Befehl | Formel | Bedeutung |
|---|---|---|---|---|
| `soc` | 7E4 | 220105 | `B34/2` | Anzeige-SoC |
| `bat_temp_max` / `bat_temp_min` | 7E4 | 220101 | `s8(B17)` / `s8(B18)` | Batterietemperatur °C |
| `odometer` | 7C6 | 22B002 | `B9*65536+B10*256+B11` | Kilometerstand (Kombiinstrument, meist nur bei Zündung an) |
| `range_calc` | 7E4 | 220105 | `B34/2*CAP/CONS` | **berechnete** Reichweite |
| `soc_bms` | 7E4 | 220101 | `B7/2` | BMS-SoC |
| `soh` | 7E4 | 220105 | `u16(B28,B29)/10` | State of Health |
| `charging` / `charging_dc` | 7E4 | 220101 | `bit(B12,7)` / `bit(B12,6)` | Laden aktiv / DC-Laden – **experimentell** (WiCAN; bei evDash war dieses Byte am eigenen Auto immer 0). Eine Abfrage für „Stecker drin“ ist nicht bekannt – das AC-Stecker-Bit bleibt auch mit Kabel 0 |

Quellen: [evDash](https://github.com/nickn17/evDash), [WiCAN](https://github.com/meatpiHQ/wican-fw) (`vehicle_profiles/hyundai/ioniq5-6.json`), OVMS.

### XPeng G9 (auch G6/P5/P7/X9)

Batteriemanagement über Header `704`, Antwort von `784`. Init: `ATZ ATE0 ATL0 ATS0 ATH0 ATSP6 ATAT1 ATAL ATCRA784 ATFCSH704 ATFCSD300000 ATFCSM1`.
**Wichtig:** `ATFCSM1` (eigene Flow-Control) funktioniert nur, wenn vorher `ATFCSH` **und** `ATFCSD` gesetzt sind. Im WiCAN-Profil (und bei uns bis 0.3.10) fehlt `ATFCSD300000` – der ELM327 lehnt `ATFCSM1` dann ab, schickt die Flow-Control an die falsche Adresse (`77C` statt `704`) und mehrteilige Antworten kommen nie vollständig an. Ab 0.3.11 werden gespeicherte Profile automatisch repariert.
Die Grundlage ist das WiCAN-Profil `xpeng_g6.json`. Korrigiert habe ich es nach [XPCarData](https://github.com/stevelea/xpcardata); dort sind die Werte am G6 geprüft:

- **SoH** liegt auf `22110A`. Bei WiCAN steht `22011A`, das ist vermutlich ein Tippfehler.
- **Kilometerstand** besteht aus 3 Bytes, nicht 2.
- **Reichweite** kommt direkt aus dem Batteriemanagement (`221118`). Das ist allerdings der **CLTC**-Wert, der meist 15–20 % über WLTP liegt.

| ID | Befehl | Formel | Bedeutung |
|---|---|---|---|
| `soc` | 221109 | `u16(B3,B4)/10` | Ladestand |
| `bat_temp_max` / `bat_temp_min` | 221107 / 221108 | `B3-40` | Batterietemperatur °C |
| `odometer` | 220101 | `B3*65536+B4*256+B5` | Kilometerstand |
| `range_cltc` | 221118 | `u16(B3,B4)` | Reichweite CLTC |
| `range_calc` | 221109 | `u16(B3,B4)/10*CAP/CONS` | berechnete Reichweite |
| `soh` | 22110A | `u16(B3,B4)/10` | State of Health |
| `charge_limit` | 221130 | `u16(B3,B4)-10` | Ladelimit % |
| `charge_status` | 22112D | `B3` (0 = nein, 2/4 = DC, 3 = AC) | Ladestatus |
| `hv_voltage` / `hv_current` | 221101 / 221103 | | HV-Spannung / -Strom |

**Wichtig:** Alle Werte stammen vom G6, am G9 ist nichts davon verifiziert, beim G9 MY25 erst recht nicht. Prüfe jeden Wert per Test-Button gegen die Anzeige im Auto. Kommt `NO DATA` oder eine negative Antwort (NRC 0x31), verwendet der G9 dort eine andere Adresse.

### XPeng G6 / P7+

Eigene Profile `g6` und `p7plus` mit denselben Werten wie beim G9. Für den G6 sind sie von XPCarData verifiziert. Für den P7+ sind keine PIDs veröffentlicht, das Profil ist deshalb **experimentell**. Bitte jeden Wert mit „Test“ prüfen. Für die berechnete Reichweite Kapazität und Verbrauch an die eigene Variante anpassen.

### Reichweite

Keines der Autos liefert die Reichweite aus dem Cockpit per OBD, weder evDash noch WiCAN noch OVMS lesen sie aus. `range_calc` schätzt sie deshalb: SoC × nutzbare Kapazität ÷ Verbrauch. Unter **Profil → Akku & Reichweite** wählst du deine Akku-Variante aus einer Liste (bekannte Größen vorbelegt, Wert von Hand korrigierbar) und trägst deinen Durchschnittsverbrauch in kWh/100 km ein. Formeln können die Variablen `CAP` (nutzbare kWh) und `CONS` (kWh/100 km) verwenden. Vorbelegte Varianten (nutzbare Kapazität): IONIQ 5 58/63/72,6/77,4/84 kWh (≈54/60/70/74/80), G9 MY25 78,2/92,2 kWh, G9 2023/24 98 kWh (≈93), G6 MY25 67,8/80,0 kWh, G6 2024 66/87,5 kWh (≈64/84), P7+ 59,6/74,9 kWh (MY25/26-Werte von ev-database.org).

### Profile aus WiCAN übernehmen

WiCAN zählt die Bytes der rohen CAN-Frames inklusive der ISO-TP-Steuerbytes, hier werden nur die Nutzdaten gezählt:

- Antwort in einem Frame: unser `B` = WiCAN `B` − 1
- Antwort über mehrere Frames: WiCAN `B2…B7` → unser `B0…B5`; danach: WiCAN `B9…B15` → `B6…B12`, `B17…B23` → `B13…B19` usw. (jedes 8. Byte fällt weg)
- WiCAN `S21` (signed) → `s8(B17)`, `[B19:B20]` → `u16(..)`, `B15:7` (Bit) → `bit(B12,7)`

### Eigene PIDs hinzufügen

1. Profil → „+ PID“, dann Header, Befehl und eine erste Formel (z. B. `B3`) eintragen.
2. „Test“ drücken. Die Bytes erscheinen mit Index (B0, B1, …). `B0` ist immer das Service-Byte der Antwort, also 0x62 bei Mode 22 bzw. 0x41 bei Mode 01.
3. Die Formel anpassen, bis der Wert plausibel ist. Min/Max dienen als Plausibilitätsfilter, damit Ausreißer verworfen werden.
4. Häkchen bei „Aktiv“ setzen und „Profil speichern“. Home Assistant legt den Sensor automatisch an.

## Watchdog & sicherer Modus

- **Hänger:** Reagiert die Hauptschleife länger als 60 s nicht, startet der ESP32 automatisch neu.
- **Verbindung:** Ist das WLAN 15 min oder MQTT 20 min weg, startet er ebenfalls neu. Ausnahme: Jemand ist gerade mit dem Setup-WLAN verbunden.
- **Sicherer Modus:** Nach **3 Abstürzen bzw. Watchdog-Resets in Folge** startet die Firmware ohne Bluetooth und Abfragen. Die Weboberfläche bleibt erreichbar, und die Status-Seite zeigt einen roten Hinweis. Wenn du Profil bzw. Einstellungen korrigiert hast, führt „Normal neu starten“ zurück in den Normalbetrieb. Nach 3 min stabilem Betrieb setzt sich der Zähler zurück, ebenso durch Aus- und Einschalten.
- Unter *System* steht der Grund des letzten Neustarts, z. B. „Task-Watchdog (Firmware hing)“ oder „Unterspannung“.

## MQTT

| Topic | Inhalt |
|---|---|
| `obd2mqtt/<pid-id>` | Messwert (retained), z. B. `obd2mqtt/soc` → `54.5` |
| `obd2mqtt/voltage_12v` | 12-V-Spannung am Dongle |
| `obd2mqtt/car` | `online` / `offline` – Dongle erreichbar |
| `obd2mqtt/polling` | `ON` / `OFF` – Hauptschalter der automatischen Abfrage; schalten über `obd2mqtt/polling/set` (in HA: Schalter „Abfrage“) |
| `obd2mqtt/battery_low` | `ON` / `OFF` – 12-V-Batterie unter der Warnschwelle (Standard 12,0 V) |
| `obd2mqtt/car_awake` | `ON` / `OFF` – Auto wach (12 V über der Schlafschwelle, d. h. DC/DC-Wandler aktiv) |
| `obd2mqtt/ble_rssi` | BLE-Signalstärke |
| `obd2mqtt/last_error` | letzter Fehler |
| `obd2mqtt/status` | `online` / `offline` (Last Will der Bridge) |
| `obd2mqtt/profile` | Name des aktiven Fahrzeugprofils, z. B. `XPeng G9` (retained) |
| `obd2mqtt/profile/attributes` | Details als JSON: `{"id":"g9","name":"XPeng G9","model":"…","active_pids":[…],"firmware":"…"}` |
| `obd2mqtt/state` | Alle aktuellen Werte inkl. Profil als **ein** JSON, nach jedem Abfragezyklus: `{"profile":"XPeng G9","profile_id":"g9","time":"…","values":{"soc":71.5,…},"voltage_12v":12.6}` |

Discovery läuft unter `homeassistant/sensor/obd2mqtt_xxxxxx/<id>/config`.

## Hinweise

- **Auto schlafen lassen:** Jede OBD-Anfrage weckt das Auto. Liegt die 12-V-Spannung unter 13,2 V (Auto aus, lädt nicht), schickt die Bridge keine Anfragen und beobachtet nur die Spannung; sobald das Auto selbst aufwacht, wird wieder normal abgefragt. Einstellbar unter Einstellungen → Abfrage.
- **12-V-Batterie**: Standardmäßig wird nur alle 120 s abgefragt, BLE dazwischen getrennt und unter 12,2 V pausiert. Wenn das Auto nicht antwortet (es schläft), wartet die Bridge 10 min. Einen Dongle mit Auto-Sleep verwenden (z. B. Vgate iCar Pro BLE, vLinker MC+/FS BLE, OBDLink CX).
- **Wann antwortet das Auto?** Beim IONIQ 5 antwortet das BMS meist nur bei Zündung an oder während des Ladens. Im Schlaf kommt `NO DATA`. Der letzte Wert bleibt in Home Assistant erhalten (retained).
- Nur **BLE**-Dongles (Bluetooth 4.0+) werden unterstützt, keine Bluetooth-Classic-Adapter (SPP).
- Befehle werden mit `ATH0`/`ATS0`/`ATCAF1` erwartet (in den Init-Befehlen gesetzt).

## Projektstruktur

```
platformio.ini          Build-Konfiguration (esp32dev / esp32s3 / esp32c3)
src/main.cpp            WLAN, Setup-AP, mDNS, Hauptschleife
src/elm_ble.*           BLE-Verbindung, UUID-Erkennung, ELM-Befehle, Scan
src/obd_parse.*         Antwort-Parser (Single/Multi-Frame) und Formel-Auswertung
src/poller.*            Abfrage-Scheduler, 12-V-Schutz, Jobs aus der Web-UI
src/mqtt_ha.*           MQTT und Home-Assistant-Discovery
src/web.*, web_ui.h     REST-API und Weboberfläche
src/config.*            Konfiguration und Profile (LittleFS)
src/default_profiles.h  Werksprofile IONIQ 5, G6, G9, P7+
lib/tinyexpr            Formel-Parser (zlib-Lizenz, codeplea/tinyexpr)
```

## Haftungsausschluss

Privates Hobbyprojekt, Beta-Software, Nutzung **auf eigene Gefahr**. Keine Gewährleistung oder Haftung für Schäden an Fahrzeug, 12-V-Batterie, Dongle oder sonstiger Hardware, soweit gesetzlich zulässig. Die Werksprofile senden nur Diagnose-**Lese**anfragen (UDS 0x22 / OBD2 Mode 01); über das Terminal lassen sich aber beliebige Befehle senden – auf eigene Verantwortung. Werte ohne Gewähr, maßgeblich ist die Anzeige im Fahrzeug. Ein dauerhaft gesteckter Dongle verbraucht Strom; bei längerer Standzeit abziehen.

Keine Verbindung zu XPeng, Hyundai, Kia, MeatPi oder Dongle-Herstellern. Marken- und Produktnamen gehören ihren Inhabern und dienen nur der Beschreibung.

Die Firmware ist mit KI-Unterstützung entstanden.

## Danke

[evDash](https://github.com/nickn17/evDash), [WiCAN](https://github.com/meatpiHQ/wican-fw), [XPCarData](https://github.com/stevelea/xpcardata), OVMS, xpeng-wican-evcc und die Community im ABRP-Feedbackboard und in den Foren.

## Lizenz

[MIT](LICENSE). Verwendete Bibliotheken unterliegen ihren eigenen Lizenzen: NimBLE-Arduino (Apache 2.0), ESPAsyncWebServer / AsyncTCP (LGPL 3.0), ArduinoJson (MIT), PubSubClient (MIT), tinyexpr (zlib, liegt unter `lib/tinyexpr`).
