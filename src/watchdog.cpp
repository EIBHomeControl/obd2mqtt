#include "watchdog.h"
#include <esp_system.h>
#include <esp_task_wdt.h>
#include "log.h"
#include "i18n.h"

namespace Watchdog {

static const uint32_t MAGIC = 0x0BD2A5E1;
static const uint32_t TASK_WDT_S = 60;
static const uint32_t WIFI_MAX_MS = 15UL * 60 * 1000;   // WLAN 15 min weg → Neustart
static const uint32_t MQTT_MAX_MS = 20UL * 60 * 1000;   // MQTT 20 min weg → Neustart
static const uint32_t STABLE_MS = 3UL * 60 * 1000;      // 3 min stabil → Absturzzähler löschen
static const uint32_t SAFE_AFTER = 3;                   // ab 3 Abstürzen in Folge: sicherer Modus

// überlebt Software-Resets (nicht aber Stromausfall)
RTC_NOINIT_ATTR static uint32_t rtcMagic;
RTC_NOINIT_ATTR static uint32_t rtcCrashes;
RTC_NOINIT_ATTR static char rtcWhy[64];
RTC_NOINIT_ATTR static char rtcStep[32];
static String hang;

static bool safe = false;
static bool armed = false;
static bool stable = false;
static uint32_t wifiDownSince = 0, mqttDownSince = 0;
static String reason;
static esp_reset_reason_t rstCode = ESP_RST_UNKNOWN;
static String rstWhy;

static const char* rawReason(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:  return T("Einschalten", "Power on");
    case ESP_RST_SW:       return T("Software-Neustart", "Software restart");
    case ESP_RST_PANIC:    return T("Absturz (Exception/Panic)", "Crash (exception/panic)");
    case ESP_RST_INT_WDT:  return "Interrupt-Watchdog";
    case ESP_RST_TASK_WDT: return T("Task-Watchdog (Firmware hing)", "Task watchdog (firmware hung)");
    case ESP_RST_WDT:      return "Watchdog";
    case ESP_RST_BROWNOUT: return T("Unterspannung (Brownout) – Netzteil/Kabel prüfen", "Brownout – check power supply/cable");
    case ESP_RST_DEEPSLEEP:return "Deep sleep";
    case ESP_RST_EXT:      return T("Reset-Taste", "Reset button");
    default:               return T("unbekannt", "unknown");
  }
}

void begin() {
  esp_reset_reason_t r = esp_reset_reason();
  if (rtcMagic != MAGIC || r == ESP_RST_POWERON || r == ESP_RST_BROWNOUT) {
    rtcMagic = MAGIC;
    rtcCrashes = 0;
    rtcWhy[0] = 0;
    rtcStep[0] = 0;
  }
  bool crash = r == ESP_RST_PANIC || r == ESP_RST_INT_WDT || r == ESP_RST_TASK_WDT || r == ESP_RST_WDT;
  if (crash) rtcCrashes++;

  rstCode = r;
  if (r == ESP_RST_SW && rtcWhy[0]) rstWhy = rtcWhy;
  rtcWhy[0] = 0;
  reason = resetReason();

  rtcStep[sizeof(rtcStep) - 1] = 0;
  if (crash && rtcStep[0]) hang = rtcStep;
  rtcStep[0] = 0;

  safe = rtcCrashes >= SAFE_AFTER;
  logf(T("Letzter Neustart: %s%s", "Last restart: %s%s"), reason.c_str(),
       crash ? (String(" (") + rtcCrashes + T(". in Folge)", " in a row)")).c_str() : "");
  if (hang.length()) logf(T("Firmware hing bei: %s", "Firmware hung at: %s"), hang.c_str());
  if (safe) logf(T("SICHERER MODUS: %lu Abstürze in Folge – Bluetooth/Abfragen deaktiviert", "SAFE MODE: %lu crashes in a row – Bluetooth/polling disabled"), (unsigned long)rtcCrashes);
}

void armTaskWdt() {
#if ESP_IDF_VERSION_MAJOR >= 5
  esp_task_wdt_config_t c = {.timeout_ms = TASK_WDT_S * 1000, .idle_core_mask = 0, .trigger_panic = true};
  esp_task_wdt_reconfigure(&c);
#else
  esp_task_wdt_init(TASK_WDT_S, true);
#endif
  if (esp_task_wdt_add(NULL) == ESP_OK) armed = true;
  logf(T("Watchdog aktiv (Hänger > %lus → Neustart)", "Watchdog active (hang > %lus → restart)"), (unsigned long)TASK_WDT_S);
}

void feed() {
  if (armed) esp_task_wdt_reset();
}

void restart(const char* why) {
  strlcpy(rtcWhy, why, sizeof(rtcWhy));
  logf(T("Neustart: %s", "Restart: %s"), why);
  logFlush();
  delay(300);
  ESP.restart();
}

void clearCrashCounter() { rtcCrashes = 0; }

void step(const char* what) {
  if (!what) { rtcStep[0] = 0; return; }
  if (strncmp(rtcStep, what, sizeof(rtcStep) - 1) == 0) return;
  strlcpy(rtcStep, what, sizeof(rtcStep));
}
const char* hangStep() { return hang.c_str(); }

void loop(bool wifiUp, bool mqttConfigured, bool mqttUp, bool apClients) {
  feed();
  uint32_t now = millis();
  if (!stable && now > STABLE_MS) {
    stable = true;
    if (rtcCrashes) logf("%s", T("System stabil – Absturzzähler zurückgesetzt", "System stable – crash counter reset"));
    rtcCrashes = 0;
  }
  // WLAN weg (und niemand im Setup-AP) → Neustart
  if (!wifiUp && !apClients) {
    if (!wifiDownSince) wifiDownSince = now;
    if (now - wifiDownSince > WIFI_MAX_MS) restart(T("WLAN 15 min nicht verbunden", "WiFi not connected for 15 min"));
  } else wifiDownSince = 0;
  // MQTT weg obwohl WLAN da → Neustart
  if (wifiUp && mqttConfigured && !mqttUp) {
    if (!mqttDownSince) mqttDownSince = now;
    if (now - mqttDownSince > MQTT_MAX_MS) restart(T("MQTT 20 min nicht verbunden", "MQTT not connected for 20 min"));
  } else mqttDownSince = 0;
}

bool safeMode() { return safe; }
uint32_t crashCount() { return rtcCrashes; }
const char* resetReason() {   // in der aktuell eingestellten Sprache
  static String s;
  s = rstWhy.length() ? String("Watchdog: ") + rstWhy : String(rawReason(rstCode));
  return s.c_str();
}

}  // namespace Watchdog
