# Changelog

## 0.3.15
- New: Profile → “Battery & range” – pick the battery variant from a list (known sizes pre-filled: IONIQ 5 58–84 kWh, G9, G6, P7+), capacity can be corrected by hand, consumption in kWh/100 km, calculated full range shown
- Formulas can use the variables CAP (usable kWh) and CONS (kWh/100 km); the range formulas of the factory profiles use them (“Test” uses the values from the form)
- Stored profiles are converted: old range formula “…/100*kWh/consumption” → CAP/CONS with the values taken over, battery selection list added
- IONIQ 5 factory profile: ATAT0 (no adaptive timing) and explicit flow control (ATFCSH7E4, ATFCSD300000, ATFCSM1) – long responses like 220101 almost never arrived complete with an “OBDII v1.5” clone, now 6 of 7 in a test; unmodified IONIQ profiles are converted automatically
- With user-defined flow control (ATFCSM1) the flow-control address follows every header change automatically (e.g. 7E4 → 7C6 for the odometer)
- MQTT: battery capacity and consumption in the profile attributes

## 0.3.14
- Fix: in sleep mode “BLE disconnected (reason 0x216)” was still logged every minute – filled the log in about 4 hours and pushed out older entries

## 0.3.13
- Lower memory use: log download and diagnostic report are sent in 1 KB chunks instead of being built completely in RAM – previously “Download/Copy” briefly needed up to ~40 KB of heap (measured minimum 25 KB)
- Diagnostic report is fetched via /api/diag (no longer inside the job JSON)

## 0.3.12
- New: diagnostic report (Status → Diagnostics) – fresh connection, every init response (rejected commands marked), ATI/AT@1/ATRV/ATDPN, every query of the profile with raw response, response time, indexed bytes and calculated values, summary; copy/download; without WiFi, IP and password data (dongle MAC shortened)
- New: diagnostic log for 2 hours – every request with raw response, init responses, voltage/protocol/MTU on each connection; switches itself off
- Init commands rejected by the dongle (“?”) are now always logged
- Manual: guide “Diagnostic report for beta testers”

## 0.3.11
- XPeng G6/G9/P7+: init sequence fixed – ATFCSD300000 added before ATFCSM1 (otherwise the ELM327 rejects the user-defined flow control and multi-frame responses never arrive; bug from the WiCAN profile), plus ATAL
- Stored profiles are repaired automatically at startup (missing ATFCSD added, 7DF queries disabled when a fixed receive filter ATCRA is set) – noted in the log
- XPeng factory profiles: “SoC (OBD2 standard)” removed – it could never answer because of the receive filter on 784
- Sleep mode with hysteresis like WiCAN: sleep below the threshold, awake only from threshold + 0.1 V
- Polling while asleep (every N min) only above 11.9 V
- New: 12V warning – Home Assistant sensor “12V low” (<base>/battery_low) below an adjustable threshold (default 12.0 V)

## 0.3.10
- BLE: larger packets (MTU 247) and shorter connection interval – more throughput against “Incomplete response” with long responses and weak signal; MTU is shown in the log
- Sleep mode: the check every minute no longer writes “BLE connected/disconnected” to the log each time (fewer flash writes)
- IONIQ 5 factory profile: “AC plug” removed – stays 0 even with the charging cable plugged in; existing profiles: disable or delete the value

## 0.3.9
- IONIQ 5 factory profile: “Charging BMS” (220106) removed – the bit is 1 whenever the car is awake (12 V top-up), even without a charging cable; existing profiles: disable or delete the value
- Sleep mode: if the 12 V voltage cannot be read, a sleeping car is left alone (previously it was polled and woken up)
- Log shows when “Poll now” wakes a sleeping car

## 0.3.8
- New: main switch “Polling ON/OFF” – on the status page and in Home Assistant as a switch (<base>/polling, command to <base>/polling/set). When OFF the bridge no longer connects to the car; Test, terminal and “Poll now” keep working. The setting survives a restart
- New: let the car sleep – if the 12 V voltage is below 13.2 V (car off, not charging), no requests are sent to the car, only the voltage at the dongle is measured. Previously polling every 2 min kept waking the IONIQ 5 (12 V jumped to 14.7 V each time)
- Settings → Polling: threshold “Car asleep below” and optional “Poll while car is asleep” (every N min); “Poll now” always polls
- Status shows “Car awake/asleep” with voltage; Home Assistant: new sensor “Car awake” (<base>/car_awake)
- Fix: the 10 min pause “No valid response” also triggered on incomplete responses although the car was awake – now only when the car does not respond at all
- IONIQ 5 factory profile: longer response timeout (ATSTFF) against incomplete multi-frame responses (existing profiles: change init command ATST96 → ATSTFF or restore factory profiles)
- Less CAN traffic: identical requests are sent only once per poll cycle (previously e.g. 220105 up to 3× per cycle)
- IONIQ 5 factory profile: new experimental values “Charging”, “DC charging”, “AC plug” (220101 byte 12, per WiCAN) and “Charging BMS” (220106 byte 27, per evDash) – please verify with Test

## 0.3.7
- Settings → WiFi: IP address automatic (DHCP) or static (IP, subnet mask, gateway, DNS); button “Use current values”
- Fallback: if the static IP does not work at startup (no WiFi connection or MQTT broker not reachable), the bridge automatically gets an address via DHCP – shown in the log, under System and in the settings
- Invalid entries (e.g. gateway not in subnet) are detected when saving or at startup

## 0.3.6
- Fix: crash loop caused by a full file system – the log files could use ~96 KB of 128 KB; now max. 2 × 16 KB, and the log never uses more than 70 % of the flash (profiles/settings take priority)
- Fix: log files are only read/written under a lock – downloading could collide with simultaneous writing/rotation
- Self-healing: after 2 crashes in a row the older log is deleted, after 3 the current one too; same at startup when the file system is too full; old 48 KB logs are removed on update
- System: free heap with minimum and largest block, plus file system usage
- Status in safe mode shows the MQTT connection correctly again
- Log lines right after startup use local time instead of UTC

## 0.3.5
- MQTT: active vehicle profile at <base>/profile (+ details as JSON at <base>/profile/attributes), in Home Assistant as sensor “Vehicle profile”
- MQTT: all values including profile and timestamp as one JSON at <base>/state after each poll cycle

## 0.3.4
- Time zone as an understandable selection list (e.g. “Central Europe – Germany, Austria, Switzerland …”) instead of a POSIX string; a custom value is still possible
- Current time on the device is shown below the time zone
- Dongle is shown with its name: Bluetooth name, detected type and ELM version (e.g. “vLinker MC · vLinker / iOS-Vlink · ELM327 v2.2”), also in the status and in the log

## 0.3.3
- Profile selection shows readable names instead of file names (e.g. “Hyundai IONIQ 5 / Kia EV6”, “XPeng G9”), sorted, active profile marked with ✓
- “Save copy” only asks for a name – the internal file name is generated automatically
- Factory profiles renamed: “Hyundai IONIQ 5 / Kia EV6”, “XPeng P7+ (experimental)”

## 0.3.2
- Factory profiles: all values enabled by default (IONIQ 5, G9, G6, P7+)
- Profile table: checkbox in the “Active” column header switches all values on/off

## 0.3.1
- New factory profiles: XPeng G6 and XPeng P7+ (P7+ experimental)
- Existing devices get the new profiles automatically after the update

## 0.3.0
- Complete interface, device messages and manual in German and English – language selector at the top right
- Language is stored on the device; log, error messages and Home Assistant names follow the selection
- Factory profiles with German or English value names
- Bilingual changelog (CHANGELOG.md / CHANGELOG.en.md)

## 0.2.5
- Back up settings and profiles (JSON file, optionally including passwords)
- Restore a backup – settings and/or profiles, followed by an automatic restart

## 0.2.4
- User manual built into the device (link “Help” at the top right or /hilfe)
- Manual with search box and live check of your own system with links to the matching help

## 0.2.3
- Log with real time of day (NTP, configurable time zone; before sync the uptime “+hh:mm:ss”)
- Store log permanently in flash (survives restarts, rotates at 48 KB), can be disabled
- Log: copy, download, download older log, clear
- Changelog in the System tab

## 0.2.2
- Warm-up after dongle reset (ATRV + pause) – fixes CAN ERROR on the Test button
- Single retry on CAN ERROR
- BLE stays connected for 60 s after Test/Terminal

## 0.2.1
- Late ELM prompts (“>”) are ignored – fixes truncated responses
- Wait until the dongle is quiet before each command
- Up to 2 retries on incomplete responses, raw response in the log

## 0.2.0
- Watchdog: hang > 60 s, WiFi 15 min or MQTT 20 min down → restart
- Safe mode after 3 crashes in a row (no BLE, web UI reachable)
- Reason of the last restart in the System tab

## 0.1.10
- Fix: “Restore factory profiles” and “Delete profile” reported “Invalid profile name”

## 0.1.9
- Incomplete multi-frame responses are detected and retried
- IONIQ 5: init command ATST96 (longer timeout)
- Clearer log message for scheduled BLE disconnects

## 0.1.8
- Status LED removed (ESP32-DevKitC V4 has no controllable LED)

## 0.1.7
- Firmware update with real progress, error messages and waiting for the restart
- Check for factory.bin, BLE polling pauses during the update

## 0.1.5 – 0.1.6
- Version number in the file name and in the web interface
- LED test (removed later)

## 0.1.4
- XPeng G9: SoH 22110A, odometer 3 bytes, CLTC range 221118, charge limit/charge status

## 0.1.3
- Battery temperature, odometer, calculated range; G9 profile based on WiCAN

## 0.1.1
- WiFi scan in the settings

## 0.1.0
- First version: BLE ELM327 → MQTT with Home Assistant discovery, web interface, profiles, test, terminal, OTA
