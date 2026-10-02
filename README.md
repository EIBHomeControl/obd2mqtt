# OBD2MQTT – BLE-OBD2 → MQTT → Home Assistant

> **Beta.** Tested on a Hyundai IONIQ 5. The XPeng profiles (G6, G9, P7+) are still **untested** – feedback is very welcome (GitHub issue or forum).
> 🇩🇪 *Deutsche Version: [README.de.md](README.de.md)*

**[⬇ Download: latest firmware (Releases)](../../releases/latest)** · [Manual](docs/manual_en.html) · [Changelog](CHANGELOG.en.md)

ESP32 firmware that connects via Bluetooth LE to an ELM327-compatible OBD2 dongle in the car, polls configurable PIDs and publishes the values via MQTT (with Home Assistant auto-discovery). Everything is configured through a web interface – no YAML, no programming.

**Recommended setup:** the ESP32 stays in the garage (on WiFi, USB power supply), the dongle stays plugged into the car. As soon as the car is within BLE range (typically 5–15 m), values arrive in Home Assistant.

```
Car (OBD2 dongle) ──Bluetooth LE──► ESP32 in the garage ──WiFi──► MQTT ──► Home Assistant / evcc
```

## Features

- BLE client for ELM327 dongles (auto-detects common UUIDs: FFF0, FFE0, 18F0, vLinker; can be overridden manually)
- Vehicle profiles as JSON (pre-installed: Hyundai IONIQ 5 / Kia EV6, XPeng G6, G9 and P7+), editable in the web UI
- Web interface and built-in manual in English and German, backup/restore of all settings, log with real time (NTP)
- PIDs with freely definable formulas (`B34/2`, `u16(B28,B29)/10`, `s16(..)`, `s8(..)`, `bit(x,n)`)
- **Test button** per PID: shows every response byte with its index so you can easily find byte positions
- ELM327 terminal in the browser
- MQTT auto-discovery: sensors appear automatically as a device in Home Assistant
- 12 V protection (ATRV), pause when the car doesn't respond, BLE disconnects between polls so the dongle can sleep
- Setup access point with captive portal, mDNS (`http://obd2mqtt.local`), OTA updates, optional password protection
- Watchdog with automatic restart and safe mode

## What you need

- **ESP32 dev board** (classic ESP32, e.g. ESP32-DevKitC V4) with a USB power supply, within BLE range of the car
- **Bluetooth LE OBD2 dongle**, ELM327-compatible, e.g. vLinker MC+/FS, Veepeak BLE+, OBDLink CX or Vgate iCar Pro BLE. *Classic Bluetooth (SPP) dongles without "LE" do not work.*
- Home Assistant with an MQTT broker (e.g. the Mosquitto add-on)

## Flashing

**Option A – no toolchain (classic ESP32):**
Download `obd2mqtt-vX.Y.Z-esp32-factory.bin` from [Releases](../../releases/latest) and flash it at **address 0x0** with a web flasher in Chrome/Edge (e.g. https://espressif.github.io/esptool-js/).

**Option B – PlatformIO (all boards):**
```bash
pio run -e esp32dev -t upload      # classic ESP32
pio run -e esp32s3 -t upload       # ESP32-S3
pio run -e esp32c3 -t upload       # ESP32-C3
pio device monitor
```

Later updates: web UI → System → Firmware update with `obd2mqtt-vX.Y.Z-esp32-ota.bin` from the releases (after `pio run` it is copied to `firmware/obd2mqtt-vX.Y.Z-<env>-ota.bin` automatically).

## First setup

1. After the first boot the ESP32 opens the WiFi network **`OBD2MQTT-xxxx`** (password `obd2mqtt`). Connect and open http://192.168.4.1. Switch the language to English at the top right.
2. **Settings → WiFi**: enter SSID and password, then **MQTT**: broker, user and password.
3. **BLE dongle**: park the car with the dongle plugged in nearby, then click "Scan for dongles". Entries marked with ★ are most likely OBD adapters. Click the entry.
4. "Save & restart". Afterwards the interface is available at `http://obd2mqtt.local` or the assigned IP address.
5. **Profile**: select the matching profile and click "Activate". Click "Test" next to `soc` to check that a value comes back.

## Profiles

All values of the factory profiles are enabled by default; you can switch individual values off in the profile.

### Hyundai IONIQ 5 (also Kia EV6 / IONIQ 6, E-GMP)

| ID | Header | Command | Formula | Meaning |
|---|---|---|---|---|
| `soc` | 7E4 | 220105 | `B34/2` | displayed SoC |
| `bat_temp_max` / `bat_temp_min` | 7E4 | 220101 | `s8(B17)` / `s8(B18)` | battery temperature °C |
| `odometer` | 7C6 | 22B002 | `B9*65536+B10*256+B11` | odometer (instrument cluster, usually only with ignition on) |
| `range_calc` | 7E4 | 220105 | `B34/2/100*74/0.19` | **calculated** range |
| `soc_bms` | 7E4 | 220101 | `B7/2` | BMS SoC |
| `soh` | 7E4 | 220105 | `u16(B28,B29)/10` | state of health |
| `charging` / `charging_dc` / `ac_plug` | 7E4 | 220101 | `bit(B12,7)` / `bit(B12,6)` / `bit(B12,5)` | charging active / DC charging / AC plug – **experimental** (WiCAN; evDash reported this byte as always 0 on its car) |
| `charging_bms` | 7E4 | 220106 | `bit(B27,0)` | charging active – **experimental** (evDash) |

Sources: [evDash](https://github.com/nickn17/evDash), [WiCAN](https://github.com/meatpiHQ/wican-fw) (`vehicle_profiles/hyundai/ioniq5-6.json`), OVMS.

### XPeng G9 (also G6/P5/P7/X9)

Battery management via header `704`, response from `784` (`ATCRA784`, `ATFCSH704`, `ATFCSM1` in the init commands).
The basis is the WiCAN profile `xpeng_g6.json`, corrected according to [XPCarData](https://github.com/stevelea/xpcardata), where the values were verified on a G6:

- **SoH** is at `22110A`. WiCAN lists `22011A`, which is most likely a typo.
- **Odometer** is 3 bytes, not 2.
- **Range** comes directly from the battery management (`221118`). Note that this is the **CLTC** value, which is usually 15–20 % above WLTP.

| ID | Command | Formula | Meaning |
|---|---|---|---|
| `soc` | 221109 | `u16(B3,B4)/10` | state of charge |
| `bat_temp_max` / `bat_temp_min` | 221107 / 221108 | `B3-40` | battery temperature °C |
| `odometer` | 220101 | `B3*65536+B4*256+B5` | odometer |
| `range_cltc` | 221118 | `u16(B3,B4)` | range (CLTC) |
| `range_calc` | 221109 | `u16(B3,B4)/10/100*93/0.20` | calculated range |
| `soh` | 22110A | `u16(B3,B4)/10` | state of health |
| `charge_limit` | 221130 | `u16(B3,B4)-10` | charge limit % |
| `charge_status` | 22112D | `B3` (0 = no, 2/4 = DC, 3 = AC) | charging status |
| `hv_voltage` / `hv_current` | 221101 / 221103 | | HV voltage / current |

**Important:** all values come from the G6. None of them is verified on the G9, let alone the G9 MY25. Check every value with the Test button against the display in the car. If you get `NO DATA` or a negative response (NRC 0x31), the G9 uses a different address for that value.

### XPeng G6 / P7+

Separate profiles `g6` and `p7plus` with the same values as the G9. For the G6 they are verified by XPCarData. For the P7+ no PIDs have been published, so that profile is **experimental**. Please check every value with "Test". For the calculated range, adjust capacity and consumption to your variant.

### Range

None of these cars report the dashboard range via OBD – neither evDash nor WiCAN nor OVMS read it. `range_calc` therefore estimates it: SoC × usable capacity (kWh) ÷ consumption (kWh/km). Put your own values into the formula, e.g. `…*63/0.17` for an IONIQ 5 with 63 kWh. Alternatively, calculate it as a template sensor in Home Assistant.

### Converting WiCAN profiles

WiCAN counts the bytes of the raw CAN frames including the ISO-TP control bytes; here only the payload is counted:

- Single-frame response: our `B` = WiCAN `B` − 1
- Multi-frame response: WiCAN `B2…B7` → our `B0…B5`; then WiCAN `B9…B15` → `B6…B12`, `B17…B23` → `B13…B19` etc. (every 8th byte is dropped)
- WiCAN `S21` (signed) → `s8(B17)`, `[B19:B20]` → `u16(..)`, `B15:7` (bit) → `bit(B12,7)`

### Adding your own PIDs

1. Profile → "+ PID", then enter header, command and a first formula (e.g. `B3`).
2. Click "Test". The bytes are shown with their index (B0, B1, …). `B0` is always the service byte of the response, i.e. 0x62 for mode 22 or 0x41 for mode 01.
3. Adjust the formula until the value is plausible. Min/max act as a plausibility filter so outliers are discarded.
4. Tick "Active" and click "Save profile". Home Assistant creates the sensor automatically.

## Watchdog & safe mode

- **Hang:** if the main loop doesn't respond for more than 60 s, the ESP32 restarts automatically.
- **Connectivity:** if WiFi is gone for 15 min or MQTT for 20 min, it restarts as well – unless someone is connected to the setup WiFi.
- **Safe mode:** after **3 crashes or watchdog resets in a row** the firmware starts without Bluetooth and polling. The web interface stays reachable and the status page shows a red notice. Once you have fixed the profile or settings, "Restart normally" returns to normal operation. The counter resets after 3 min of stable operation or a power cycle.
- *System* shows the reason for the last restart, e.g. "Task watchdog (firmware hung)" or "Brownout".

## MQTT

| Topic | Content |
|---|---|
| `obd2mqtt/<pid-id>` | measured value (retained), e.g. `obd2mqtt/soc` → `54.5` |
| `obd2mqtt/voltage_12v` | 12 V voltage measured at the dongle |
| `obd2mqtt/car` | `online` / `offline` – dongle reachable |
| `obd2mqtt/polling` | `ON` / `OFF` – main switch for automatic polling; switch it via `obd2mqtt/polling/set` (in HA: switch "Polling") |
| `obd2mqtt/car_awake` | `ON` / `OFF` – car awake (12 V above the sleep threshold, i.e. DC/DC converter active) |
| `obd2mqtt/ble_rssi` | BLE signal strength |
| `obd2mqtt/last_error` | last error |
| `obd2mqtt/status` | `online` / `offline` (last will of the bridge) |
| `obd2mqtt/profile` | name of the active vehicle profile, e.g. `XPeng G9` (retained) |
| `obd2mqtt/profile/attributes` | details as JSON: `{"id":"g9","name":"XPeng G9","model":"…","active_pids":[…],"firmware":"…"}` |
| `obd2mqtt/state` | all current values incl. profile as **one** JSON after every poll cycle: `{"profile":"XPeng G9","profile_id":"g9","time":"…","values":{"soc":71.5,…},"voltage_12v":12.6}` |

Discovery is published under `homeassistant/sensor/obd2mqtt_xxxxxx/<id>/config`.

## Notes

- **Let the car sleep:** every OBD request wakes the car. If the 12 V voltage is below 13.2 V (car off, not charging), the bridge sends no requests and only watches the voltage; as soon as the car wakes up by itself, normal polling resumes. Adjustable under Settings → Polling.
- **12 V battery:** by default the car is polled only every 120 s, BLE is disconnected in between and polling pauses below 12.2 V. If the car doesn't respond (it's asleep), the bridge waits 10 min. Use a dongle with auto-sleep (e.g. Vgate iCar Pro BLE, vLinker MC+/FS BLE, OBDLink CX).
- **When does the car respond?** On the IONIQ 5 the BMS usually only responds with ignition on or while charging. While asleep you get `NO DATA`. The last value is kept in Home Assistant (retained).
- Only **BLE** dongles (Bluetooth 4.0+) are supported, no Bluetooth Classic adapters (SPP).
- Commands expect `ATH0`/`ATS0`/`ATCAF1` (set in the init commands).

## Project structure

```
platformio.ini          build configuration (esp32dev / esp32s3 / esp32c3)
src/main.cpp            WiFi, setup AP, mDNS, main loop
src/elm_ble.*           BLE connection, UUID detection, ELM commands, scan
src/obd_parse.*         response parser (single/multi-frame) and formula evaluation
src/poller.*            poll scheduler, 12 V protection, jobs from the web UI
src/mqtt_ha.*           MQTT and Home Assistant discovery
src/web.*, web_ui.h     REST API and web interface
src/help_ui.h           built-in manual (DE/EN)
src/config.*            configuration and profiles (LittleFS)
src/default_profiles.h  factory profiles IONIQ 5, G6, G9, P7+
lib/tinyexpr            formula parser (zlib license, codeplea/tinyexpr)
changelog.json          changelog source → tools/gen_changelog.py → CHANGELOG*.md + web UI
```

## Disclaimer

Private hobby project, beta software, use **at your own risk**. No warranty or liability for damage to the vehicle, 12 V battery, dongle or other hardware, to the extent permitted by law. The factory profiles only send diagnostic **read** requests (UDS 0x22 / OBD2 mode 01); the terminal, however, can send arbitrary commands – at your own responsibility. Values without guarantee; the display in the vehicle is always authoritative. A permanently plugged-in dongle draws power; unplug it if the car is parked for a longer time.

Not affiliated with XPeng, Hyundai, Kia, MeatPi or any dongle manufacturer. Brand and product names belong to their respective owners and are used for descriptive purposes only.

This firmware was developed with AI assistance.

## Thanks

[evDash](https://github.com/nickn17/evDash), [WiCAN](https://github.com/meatpiHQ/wican-fw), [XPCarData](https://github.com/stevelea/xpcardata), OVMS, xpeng-wican-evcc and the community on the ABRP feedback board and in the forums.

## License

[MIT](LICENSE). Bundled/used libraries are subject to their own licenses: NimBLE-Arduino (Apache 2.0), ESPAsyncWebServer / AsyncTCP (LGPL 3.0), ArduinoJson (MIT), PubSubClient (MIT), tinyexpr (zlib, included in `lib/tinyexpr`).
