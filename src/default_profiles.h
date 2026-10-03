#pragma once
// Werksprofile – werden beim ersten Start nach /profiles/ geschrieben
// und können danach in der Weboberfläche bearbeitet werden.
//
// Formeln: B0..Bn = Bytes der (zusammengesetzten) Antwort INKL. Service-Bytes.
//   Beispiel 220105 → Antwort "62 01 05 ..." → B0=0x62, B1=0x01, B2=0x05, B3=erstes Datenbyte
// Hilfsfunktionen: u16(hi,lo), s16(hi,lo), s8(x), bit(x,n)
//
// Umrechnung von WiCAN-Profilen (github.com/meatpiHQ/wican-fw), die rohe CAN-Frames zählen:
//   Single-Frame:  unser B = WiCAN B - 1
//   Multi-Frame:   WiCAN B2..B7 → unser B0..B5;  ab WiCAN B9: unser B = 6 + 7*(k-1) + (W - 8k - 1),
//                  k = Frame-Nr. (W div 8). Beispiel: WiCAN B41 → unser B34.

// Hyundai IONIQ 5 / Kia EV6 / IONIQ 6 (E-GMP)
// Offsets aus evDash (CarHyundaiEgmp.cpp), gegengeprüft mit WiCAN ioniq5-6.json und OVMS
static const char PROFILE_IONIQ5[] PROGMEM = R"JSON({
  "name": "Hyundai IONIQ 5 / Kia EV6", "name_en": "Hyundai IONIQ 5 / Kia EV6",
  "model": "Hyundai IONIQ 5 (E-GMP)",
  "init": ["ATZ", "ATE0", "ATL0", "ATS0", "ATH0", "ATSP6", "ATSTFF"],
  "pids": [
    { "id": "soc", "name": "SoC", "header": "7E4", "cmd": "220105",
      "formula": "B34/2", "unit": "%", "device_class": "battery",
      "interval": 120, "min": 0, "max": 100, "precision": 1, "enabled": true },
    { "id": "bat_temp_max", "name": "Batterie Temp max", "name_en": "Battery temp max", "header": "7E4", "cmd": "220101",
      "formula": "s8(B17)", "unit": "°C", "device_class": "temperature",
      "interval": 300, "min": -40, "max": 90, "precision": 0, "enabled": true },
    { "id": "bat_temp_min", "name": "Batterie Temp min", "name_en": "Battery temp min", "header": "7E4", "cmd": "220101",
      "formula": "s8(B18)", "unit": "°C", "device_class": "temperature",
      "interval": 300, "min": -40, "max": 90, "precision": 0, "enabled": true },
    { "id": "odometer", "name": "Kilometerstand", "name_en": "Odometer", "header": "7C6", "cmd": "22B002",
      "formula": "B9*65536+B10*256+B11", "unit": "km", "device_class": "distance",
      "state_class": "total_increasing", "icon": "mdi:counter",
      "interval": 1800, "min": 1, "max": 2000000, "precision": 0, "enabled": true },
    { "id": "range_calc", "name": "Reichweite (berechnet)", "name_en": "Range (calculated)", "header": "7E4", "cmd": "220105",
      "formula": "B34/2/100*74/0.19", "unit": "km", "device_class": "distance",
      "icon": "mdi:map-marker-distance",
      "interval": 120, "min": 0, "max": 800, "precision": 0, "enabled": true },
    { "id": "soc_bms", "name": "SoC (BMS roh)", "name_en": "SoC (BMS raw)", "header": "7E4", "cmd": "220101",
      "formula": "B7/2", "unit": "%", "device_class": "battery",
      "interval": 120, "min": 0, "max": 100, "precision": 1, "enabled": true },
    { "id": "soh", "name": "SoH", "header": "7E4", "cmd": "220105",
      "formula": "u16(B28,B29)/10", "unit": "%", "icon": "mdi:battery-heart-variant",
      "interval": 3600, "min": 0, "max": 100, "precision": 1, "enabled": true },
    { "id": "charging", "name": "Laden aktiv (experimentell)", "name_en": "Charging (experimental)", "header": "7E4", "cmd": "220101",
      "formula": "bit(B12,7)", "icon": "mdi:ev-station", "state_class": "",
      "interval": 120, "min": 0, "max": 1, "precision": 0, "enabled": true },
    { "id": "charging_dc", "name": "DC-Laden (experimentell)", "name_en": "DC charging (experimental)", "header": "7E4", "cmd": "220101",
      "formula": "bit(B12,6)", "icon": "mdi:ev-plug-ccs2", "state_class": "",
      "interval": 120, "min": 0, "max": 1, "precision": 0, "enabled": true }
  ]
})JSON";

// XPeng G9 (auch G6/P5/P7/X9) – BMS über Header 704, Antwort von 784.
// Quellen: WiCAN xpeng_g6.json, korrigiert nach XPCarData (github.com/stevelea/xpcardata, am G6 verifiziert):
//   SOH = 22110A (nicht 22011A wie bei WiCAN), Kilometerstand = 3 Bytes, Reichweite = 221118 (CLTC).
// Am G9 (insb. MY25) noch nicht verifiziert – jeden PID mit „Test“ gegen die Anzeige im Auto prüfen.
static const char PROFILE_G9[] PROGMEM = R"JSON({
  "name": "XPeng G9",
  "model": "XPeng G9",
  "init": ["ATZ", "ATE0", "ATL0", "ATS0", "ATH0", "ATSP6", "ATAT1", "ATAL", "ATCRA784", "ATFCSH704", "ATFCSD300000", "ATFCSM1"],
  "pids": [
    { "id": "soc", "name": "SoC", "header": "704", "cmd": "221109",
      "formula": "u16(B3,B4)/10", "unit": "%", "device_class": "battery",
      "interval": 120, "min": 0, "max": 100, "precision": 1, "enabled": true },
    { "id": "bat_temp_max", "name": "Batterie Temp max", "name_en": "Battery temp max", "header": "704", "cmd": "221107",
      "formula": "B3-40", "unit": "°C", "device_class": "temperature",
      "interval": 300, "min": -40, "max": 90, "precision": 0, "enabled": true },
    { "id": "bat_temp_min", "name": "Batterie Temp min", "name_en": "Battery temp min", "header": "704", "cmd": "221108",
      "formula": "B3-40", "unit": "°C", "device_class": "temperature",
      "interval": 300, "min": -40, "max": 90, "precision": 0, "enabled": true },
    { "id": "odometer", "name": "Kilometerstand", "name_en": "Odometer", "header": "704", "cmd": "220101",
      "formula": "B3*65536+B4*256+B5", "unit": "km", "device_class": "distance",
      "state_class": "total_increasing", "icon": "mdi:counter",
      "interval": 1800, "min": 1, "max": 2000000, "precision": 0, "enabled": true },
    { "id": "range_cltc", "name": "Reichweite (CLTC)", "name_en": "Range (CLTC)", "header": "704", "cmd": "221118",
      "formula": "u16(B3,B4)", "unit": "km", "device_class": "distance",
      "icon": "mdi:map-marker-distance",
      "interval": 300, "min": 0, "max": 1000, "precision": 0, "enabled": true },
    { "id": "range_calc", "name": "Reichweite (berechnet)", "name_en": "Range (calculated)", "header": "704", "cmd": "221109",
      "formula": "u16(B3,B4)/10/100*93/0.20", "unit": "km", "device_class": "distance",
      "icon": "mdi:map-marker-distance",
      "interval": 120, "min": 0, "max": 800, "precision": 0, "enabled": true },
    { "id": "soh", "name": "SoH", "header": "704", "cmd": "22110A",
      "formula": "u16(B3,B4)/10", "unit": "%", "icon": "mdi:battery-heart-variant",
      "interval": 3600, "min": 0, "max": 100, "precision": 1, "enabled": true },
    { "id": "charge_limit", "name": "Ladelimit", "name_en": "Charge limit", "header": "704", "cmd": "221130",
      "formula": "u16(B3,B4)-10", "unit": "%", "icon": "mdi:battery-charging-high",
      "interval": 900, "min": 0, "max": 100, "precision": 0, "enabled": true },
    { "id": "charge_status", "name": "Ladestatus (0=nein, 2/4=DC, 3=AC)", "name_en": "Charging status (0=no, 2/4=DC, 3=AC)", "header": "704", "cmd": "22112D",
      "formula": "B3", "state_class": "", "icon": "mdi:ev-station",
      "interval": 120, "min": 0, "max": 10, "precision": 0, "enabled": true },
    { "id": "hv_voltage", "name": "HV-Spannung", "name_en": "HV voltage", "header": "704", "cmd": "221101",
      "formula": "u16(B3,B4)/10", "unit": "V", "device_class": "voltage",
      "interval": 120, "min": 0, "max": 1000, "precision": 1, "enabled": true },
    { "id": "hv_current", "name": "HV-Strom (negativ = Laden)", "name_en": "HV current (negative = charging)", "header": "704", "cmd": "221103",
      "formula": "u16(B3,B4)*0.5-1600", "unit": "A", "device_class": "current",
      "interval": 120, "min": -1600, "max": 1600, "precision": 1, "enabled": true }
  ]
})JSON";
// XPeng G6 – gleiche BMS-PIDs wie G9 (704/784). Die Werte von XPCarData sind am G6 verifiziert.
// Reichweite (berechnet): Kapazität/Verbrauch an die eigene Variante anpassen (SR ca. 66 kWh, LR ca. 87,5 kWh brutto).
static const char PROFILE_G6[] PROGMEM = R"JSON({
  "name": "XPeng G6",
  "model": "XPeng G6",
  "init": ["ATZ", "ATE0", "ATL0", "ATS0", "ATH0", "ATSP6", "ATAT1", "ATAL", "ATCRA784", "ATFCSH704", "ATFCSD300000", "ATFCSM1"],
  "pids": [
    { "id": "soc", "name": "SoC", "header": "704", "cmd": "221109",
      "formula": "u16(B3,B4)/10", "unit": "%", "device_class": "battery",
      "interval": 120, "min": 0, "max": 100, "precision": 1, "enabled": true },
    { "id": "bat_temp_max", "name": "Batterie Temp max", "name_en": "Battery temp max", "header": "704", "cmd": "221107",
      "formula": "B3-40", "unit": "°C", "device_class": "temperature",
      "interval": 300, "min": -40, "max": 90, "precision": 0, "enabled": true },
    { "id": "bat_temp_min", "name": "Batterie Temp min", "name_en": "Battery temp min", "header": "704", "cmd": "221108",
      "formula": "B3-40", "unit": "°C", "device_class": "temperature",
      "interval": 300, "min": -40, "max": 90, "precision": 0, "enabled": true },
    { "id": "odometer", "name": "Kilometerstand", "name_en": "Odometer", "header": "704", "cmd": "220101",
      "formula": "B3*65536+B4*256+B5", "unit": "km", "device_class": "distance",
      "state_class": "total_increasing", "icon": "mdi:counter",
      "interval": 1800, "min": 1, "max": 2000000, "precision": 0, "enabled": true },
    { "id": "range_cltc", "name": "Reichweite (CLTC)", "name_en": "Range (CLTC)", "header": "704", "cmd": "221118",
      "formula": "u16(B3,B4)", "unit": "km", "device_class": "distance",
      "icon": "mdi:map-marker-distance",
      "interval": 300, "min": 0, "max": 1000, "precision": 0, "enabled": true },
    { "id": "range_calc", "name": "Reichweite (berechnet)", "name_en": "Range (calculated)", "header": "704", "cmd": "221109",
      "formula": "u16(B3,B4)/10/100*80/0.19", "unit": "km", "device_class": "distance",
      "icon": "mdi:map-marker-distance",
      "interval": 120, "min": 0, "max": 800, "precision": 0, "enabled": true },
    { "id": "soh", "name": "SoH", "header": "704", "cmd": "22110A",
      "formula": "u16(B3,B4)/10", "unit": "%", "icon": "mdi:battery-heart-variant",
      "interval": 3600, "min": 0, "max": 100, "precision": 1, "enabled": true },
    { "id": "charge_limit", "name": "Ladelimit", "name_en": "Charge limit", "header": "704", "cmd": "221130",
      "formula": "u16(B3,B4)-10", "unit": "%", "icon": "mdi:battery-charging-high",
      "interval": 900, "min": 0, "max": 100, "precision": 0, "enabled": true },
    { "id": "charge_status", "name": "Ladestatus (0=nein, 2/4=DC, 3=AC)", "name_en": "Charging status (0=no, 2/4=DC, 3=AC)", "header": "704", "cmd": "22112D",
      "formula": "B3", "state_class": "", "icon": "mdi:ev-station",
      "interval": 120, "min": 0, "max": 10, "precision": 0, "enabled": true },
    { "id": "hv_voltage", "name": "HV-Spannung", "name_en": "HV voltage", "header": "704", "cmd": "221101",
      "formula": "u16(B3,B4)/10", "unit": "V", "device_class": "voltage",
      "interval": 120, "min": 0, "max": 1000, "precision": 1, "enabled": true },
    { "id": "hv_current", "name": "HV-Strom (negativ = Laden)", "name_en": "HV current (negative = charging)", "header": "704", "cmd": "221103",
      "formula": "u16(B3,B4)*0.5-1600", "unit": "A", "device_class": "current",
      "interval": 120, "min": -1600, "max": 1600, "precision": 1, "enabled": true }
  ]
})JSON";

// XPeng P7+ – EXPERIMENTELL: keine veröffentlichten PIDs; angenommen wird dieselbe BMS-Adressierung (704/784)
// wie bei G6/G9. Bitte jeden Wert mit „Test“ prüfen. Kapazität für die berechnete Reichweite anpassen.
static const char PROFILE_P7PLUS[] PROGMEM = R"JSON({
  "name": "XPeng P7+ (experimentell)", "name_en": "XPeng P7+ (experimental)",
  "model": "XPeng P7+",
  "init": ["ATZ", "ATE0", "ATL0", "ATS0", "ATH0", "ATSP6", "ATAT1", "ATAL", "ATCRA784", "ATFCSH704", "ATFCSD300000", "ATFCSM1"],
  "pids": [
    { "id": "soc", "name": "SoC", "header": "704", "cmd": "221109",
      "formula": "u16(B3,B4)/10", "unit": "%", "device_class": "battery",
      "interval": 120, "min": 0, "max": 100, "precision": 1, "enabled": true },
    { "id": "bat_temp_max", "name": "Batterie Temp max", "name_en": "Battery temp max", "header": "704", "cmd": "221107",
      "formula": "B3-40", "unit": "°C", "device_class": "temperature",
      "interval": 300, "min": -40, "max": 90, "precision": 0, "enabled": true },
    { "id": "bat_temp_min", "name": "Batterie Temp min", "name_en": "Battery temp min", "header": "704", "cmd": "221108",
      "formula": "B3-40", "unit": "°C", "device_class": "temperature",
      "interval": 300, "min": -40, "max": 90, "precision": 0, "enabled": true },
    { "id": "odometer", "name": "Kilometerstand", "name_en": "Odometer", "header": "704", "cmd": "220101",
      "formula": "B3*65536+B4*256+B5", "unit": "km", "device_class": "distance",
      "state_class": "total_increasing", "icon": "mdi:counter",
      "interval": 1800, "min": 1, "max": 2000000, "precision": 0, "enabled": true },
    { "id": "range_cltc", "name": "Reichweite (CLTC)", "name_en": "Range (CLTC)", "header": "704", "cmd": "221118",
      "formula": "u16(B3,B4)", "unit": "km", "device_class": "distance",
      "icon": "mdi:map-marker-distance",
      "interval": 300, "min": 0, "max": 1000, "precision": 0, "enabled": true },
    { "id": "range_calc", "name": "Reichweite (berechnet)", "name_en": "Range (calculated)", "header": "704", "cmd": "221109",
      "formula": "u16(B3,B4)/10/100*74/0.16", "unit": "km", "device_class": "distance",
      "icon": "mdi:map-marker-distance",
      "interval": 120, "min": 0, "max": 800, "precision": 0, "enabled": true },
    { "id": "soh", "name": "SoH", "header": "704", "cmd": "22110A",
      "formula": "u16(B3,B4)/10", "unit": "%", "icon": "mdi:battery-heart-variant",
      "interval": 3600, "min": 0, "max": 100, "precision": 1, "enabled": true },
    { "id": "charge_limit", "name": "Ladelimit", "name_en": "Charge limit", "header": "704", "cmd": "221130",
      "formula": "u16(B3,B4)-10", "unit": "%", "icon": "mdi:battery-charging-high",
      "interval": 900, "min": 0, "max": 100, "precision": 0, "enabled": true },
    { "id": "charge_status", "name": "Ladestatus (0=nein, 2/4=DC, 3=AC)", "name_en": "Charging status (0=no, 2/4=DC, 3=AC)", "header": "704", "cmd": "22112D",
      "formula": "B3", "state_class": "", "icon": "mdi:ev-station",
      "interval": 120, "min": 0, "max": 10, "precision": 0, "enabled": true },
    { "id": "hv_voltage", "name": "HV-Spannung", "name_en": "HV voltage", "header": "704", "cmd": "221101",
      "formula": "u16(B3,B4)/10", "unit": "V", "device_class": "voltage",
      "interval": 120, "min": 0, "max": 1000, "precision": 1, "enabled": true },
    { "id": "hv_current", "name": "HV-Strom (negativ = Laden)", "name_en": "HV current (negative = charging)", "header": "704", "cmd": "221103",
      "formula": "u16(B3,B4)*0.5-1600", "unit": "A", "device_class": "current",
      "interval": 120, "min": -1600, "max": 1600, "precision": 1, "enabled": true }
  ]
})JSON";


struct DefaultProfile { const char* file; const char* json; };
static const DefaultProfile DEFAULT_PROFILES[] = {
  { "ioniq5", PROFILE_IONIQ5 },
  { "g9",     PROFILE_G9 },
  { "g6",     PROFILE_G6 },
  { "p7plus", PROFILE_P7PLUS },
};
