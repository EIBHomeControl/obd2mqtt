# Changelog

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
