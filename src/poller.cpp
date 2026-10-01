#include "poller.h"
#include <WiFi.h>
#include <vector>
#include "config.h"
#include "elm_ble.h"
#include "log.h"
#include "mqtt_ha.h"
#include "obd_parse.h"
#include "i18n.h"

namespace Poller {

static std::mutex mtx;
std::mutex& mutex() { return mtx; }

struct PidState {
  double value = NAN;
  uint32_t lastOk = 0;      // millis() des letzten gültigen Werts
  uint32_t lastPoll = 0;    // millis() des letzten Versuchs (0 = nie)
  String err, raw;
};
static std::vector<PidState> states;

static String currentHeader;
static uint32_t nextConnectTry = 0;
static bool elmReady = false;         // Init-Sequenz ausgeführt
static float lastVoltage = NAN;
static String lastError;
static uint32_t lastConnectOk = 0;
static uint32_t pollCycles = 0;
static String elmVersion, cacheName, cacheKind, cacheUuids;
static bool cacheBle = false, cacheMqtt = false;   // für den Web-Task (keine BLE-Aufrufe dort)
static int cacheRssi = 0;

// Flags aus dem Web-Task
static volatile bool flagProfileReload = false;
static volatile bool flagPollNow = false;
static String pendingConfig;          // unter mtx

// Job
struct Job {
  JobType type;
  String header, cmd, formula;
  String state = "idle";
  String resultJson;
};
static Job job;
static uint32_t lastJobMs = 0;      // nach Test/Terminal Verbindung noch 60 s halten

// ---------------------------------------------------------------

static void setError(const String& e) {
  logf(T("Fehler: %s", "Error: %s"), e.c_str());
  {
    std::lock_guard<std::mutex> lk(mtx);
    lastError = e;
  }
  MqttHa::publishText("last_error", e);
}

static void resetStates() {
  std::lock_guard<std::mutex> lk(mtx);
  states.assign(profile.pids.size(), PidState());
}

static void reloadProfile() {
  Profile old = profile;
  bool ok;
  {
    std::lock_guard<std::mutex> lk(mtx);
    ok = loadProfile(cfg.profile);
    if (!ok) profile = old;
  }
  if (!ok) setError(String(T("Profil '", "Profile '")) + cfg.profile + T("' konnte nicht geladen werden", "' could not be loaded"));
  else logf(T("Profil '%s' geladen (%u PIDs)", "Profile '%s' loaded (%u PIDs)"), cfg.profile.c_str(), (unsigned)profile.pids.size());
  resetStates();
  elmReady = false;           // Init-Befehle könnten sich geändert haben
  MqttHa::requestDiscovery();
}

static bool elmCmd(const String& c, String& resp, uint32_t timeout = 0) {
  bool ok = ElmBle::command(c, resp, timeout ? timeout : cfg.cmdTimeoutMs);
  resp.trim();
  return ok;
}

static bool ensureReady(String& err) {
  if (!ElmBle::connected()) {
    elmReady = false;
    if (!ElmBle::connect(err)) return false;
    std::lock_guard<std::mutex> lk(mtx);
    lastConnectOk = millis();
  }
  if (!elmReady) {
    String r;
    for (auto& c : profile.init) {
      bool ok = elmCmd(c, r, c.startsWith("ATZ") || c.startsWith("AT Z") ? 4000 : 0);
      if (!ok) logf(T("Init '%s' ohne Prompt (Antwort: %s)", "Init '%s' without prompt (response: %s)"), c.c_str(), r.c_str());
      if (c.startsWith("ATZ")) delay(300);
    }
    currentHeader = "";
    // Aufwärmen: viele ELM-Clones liefern direkt nach dem Reset beim ersten
    // CAN-Telegramm "CAN ERROR" bzw. abgeschnittene Antworten.
    delay(300);
    elmCmd("ATRV", r);
    if (elmCmd("ATI", r) && r.length() && r.length() < 40) {   // z.B. "ELM327 v1.5"
      r.replace("\r", " "); r.trim();
      std::lock_guard<std::mutex> lk(mtx);
      elmVersion = r;
    }
    elmReady = true;
    MqttHa::publishText("car", "online");
    MqttHa::publishValue("ble_rssi", ElmBle::rssi(), 0);
  }
  return true;
}

static bool setHeader(const String& h) {
  if (h.isEmpty() || h == currentHeader) return true;
  String r;
  if (!elmCmd("ATSH" + h, r) || r.indexOf("OK") < 0) {
    currentHeader = "";
    return false;
  }
  currentHeader = h;
  return true;
}

static float readVoltage() {
  String r;
  if (!elmCmd("ATRV", r)) return NAN;
  float v = r.toFloat();   // "12.4V"
  return (v > 5 && v < 20) ? v : NAN;
}

// Führt header+cmd aus und liefert Bytes
static bool query(const String& header, const String& cmd, std::vector<uint8_t>& bytes,
                  String& raw, String& err) {
  if (!setHeader(header)) { err = "ATSH" + header + T(" fehlgeschlagen", " failed"); return false; }
  for (int attempt = 0; attempt < 3; attempt++) {   // bei unvollständiger Antwort bis zu 2× wiederholen
    if (attempt) {
      String r = raw.substring(0, 80);
      r.replace("\r", "|");
      logf(T("%s: %s – wiederhole (roh: %s)", "%s: %s – retrying (raw: %s)"), cmd.c_str(), err.c_str(), r.c_str());
      delay(500);
    }
    if (!elmCmd(cmd, raw)) {
      err = ElmBle::connected() ? T("Timeout (kein Prompt)", "Timeout (no prompt)") : T("BLE getrennt", "BLE disconnected");
      if (!ElmBle::connected()) return false;
      continue;
    }
    std::string e;
    bool ok = ObdParse::parseResponse(raw.c_str(), cmd.c_str(), bytes, e);
    err = e.c_str();
    if (ok) return true;
    // Wiederholen nur bei unvollständiger Antwort bzw. CAN-Fehler (typisch kurz nach dem Verbinden),
    // nicht bei NO DATA / negativer Antwort
    bool retry = ObdParse::lastErrKind == ObdParse::ERR_INCOMPLETE ||
                 (ObdParse::lastErrKind == ObdParse::ERR_CAN && attempt == 0);
    if (!retry) return false;
  }
  return false;
}

static bool isDue(size_t i, uint32_t now) {
  const PidDef& p = profile.pids[i];
  if (!p.enabled) return false;
  const PidState& s = states[i];
  return s.lastPoll == 0 || now - s.lastPoll >= p.interval * 1000UL;
}

static uint32_t msUntilNextDue(uint32_t now) {
  uint32_t best = UINT32_MAX;
  for (size_t i = 0; i < profile.pids.size(); i++) {
    const PidDef& p = profile.pids[i];
    if (!p.enabled) continue;
    const PidState& s = states[i];
    if (s.lastPoll == 0) return 0;
    uint32_t el = now - s.lastPoll, iv = p.interval * 1000UL;
    best = min(best, el >= iv ? 0 : iv - el);
  }
  return best;
}

// Alle aktuellen Werte inkl. Profil als ein JSON: <base>/state
static void publishStateJson() {
  JsonDocument d;
  d["profile"] = profile.name.length() ? profile.name : cfg.profile;
  d["profile_id"] = cfg.profile;
  if (timeValid()) {
    time_t tnow = time(nullptr);
    struct tm tmv;
    localtime_r(&tnow, &tmv);
    char tb[32];
    strftime(tb, sizeof(tb), "%Y-%m-%dT%H:%M:%S%z", &tmv);
    d["time"] = tb;
  }
  JsonObject v = d["values"].to<JsonObject>();
  {
    std::lock_guard<std::mutex> lk(mtx);
    for (size_t i = 0; i < profile.pids.size() && i < states.size(); i++) {
      if (!profile.pids[i].enabled || isnan(states[i].value)) continue;
      v[profile.pids[i].id] = serialized(String(states[i].value, profile.pids[i].precision));
    }
    if (!isnan(lastVoltage)) d["voltage_12v"] = serialized(String(lastVoltage, 1));
  }
  String s;
  serializeJson(d, s);
  MqttHa::publishText("state", s);
}

static void pollDue() {
  uint32_t now = millis();
  String err;
  if (!ensureReady(err)) {
    MqttHa::publishText("car", "offline");
    nextConnectTry = millis() + cfg.retrySec * 1000UL;
    logf(T("BLE: %s – neuer Versuch in %lus", "BLE: %s – retry in %lus"), err.c_str(), (unsigned long)cfg.retrySec);
    std::lock_guard<std::mutex> lk(mtx);
    lastError = err;
    return;
  }

  // 12V-Schutz
  float v = readVoltage();
  {
    std::lock_guard<std::mutex> lk(mtx);
    lastVoltage = v;
  }
  if (!isnan(v)) MqttHa::publishValue("voltage_12v", v, 1);
  if (cfg.minVoltage > 0 && !isnan(v) && v < cfg.minVoltage) {
    setError(String(T("12V zu niedrig (", "12V too low (")) + String(v, 1) + T(" V) – Abfrage pausiert", " V) – polling paused"));
    std::lock_guard<std::mutex> lk(mtx);
    for (size_t i = 0; i < states.size(); i++) if (isDue(i, now)) states[i].lastPoll = now;
    ElmBle::disconnect();
    elmReady = false;
    nextConnectTry = now + max(cfg.backoffSec, (uint32_t)900) * 1000UL;
    return;
  }

  // Fällige PIDs abfragen; gleiche header+cmd nur einmal senden
  String lastKey;
  std::vector<uint8_t> bytes;
  String raw, qerr;
  bool qok = false, anyOk = false, anyTried = false;
  for (size_t i = 0; i < profile.pids.size(); i++) {
    if (!isDue(i, now)) continue;
    const PidDef& p = profile.pids[i];
    String key = p.header + "|" + p.cmd;
    if (key != lastKey) {
      qok = query(p.header, p.cmd, bytes, raw, qerr);
      lastKey = key;
    }
    double val = NAN;
    String perr = qerr;
    bool ok = qok;
    if (ok) {
      std::string e;
      ok = ObdParse::evalFormula(p.formula.c_str(), bytes, val, e);
      perr = e.c_str();
      if (ok && ((!isnan(p.minVal) && val < p.minVal) || (!isnan(p.maxVal) && val > p.maxVal))) {
        ok = false;
        perr = String(T("Wert ", "Value ")) + String(val, 2) + T(" außerhalb Bereich – verworfen", " out of range – discarded");
      }
    }
    {
      std::lock_guard<std::mutex> lk(mtx);
      PidState& s = states[i];
      s.lastPoll = now;
      s.raw = raw.substring(0, 300);
      s.err = ok ? "" : perr;
      if (ok) { s.value = val; s.lastOk = now; }
    }
    anyTried = true;
    anyOk |= ok;
    if (ok) {
      MqttHa::publishValue(p.id, val, p.precision);
      logf("%s = %s %s", p.id.c_str(), String(val, p.precision).c_str(), p.unit.c_str());
    } else {
      setError(p.id + ": " + perr);
    }
    if (!ElmBle::connected()) { elmReady = false; break; }
  }
  pollCycles++;
  publishStateJson();
  // Keine Antwort (Auto schläft) → länger warten, damit nichts wachgehalten wird
  if (anyTried && !anyOk && cfg.backoffSec > 0) {
    nextConnectTry = millis() + cfg.backoffSec * 1000UL;
    logf(T("Keine gültige Antwort – nächster Versuch in %lus", "No valid response – next attempt in %lus"), (unsigned long)cfg.backoffSec);
    if (!cfg.keepConnected) { ElmBle::disconnect(); elmReady = false; }
  }
}

// ---------------------------------------------------------------

static void runJob() {
  Job j;
  {
    std::lock_guard<std::mutex> lk(mtx);
    if (job.state != "pending") return;
    job.state = "running";
    j = job;
  }
  JsonDocument r;
  if (j.type == JOB_SCAN) {
    ElmBle::disconnect();
    elmReady = false;
    auto list = ElmBle::scan(6000);
    JsonArray arr = r["devices"].to<JsonArray>();
    for (auto& e : list) {
      JsonObject o = arr.add<JsonObject>();
      o["mac"] = e.mac; o["name"] = e.name; o["rssi"] = e.rssi;
      o["random"] = e.addrRandom; o["obd"] = e.likelyObd;
    }
    r["ok"] = true;
  } else {
    String err;
    if (!ensureReady(err)) {
      r["ok"] = false;
      r["error"] = err;
    } else if (j.type == JOB_RAW) {
      String resp;
      bool ok = elmCmd(j.cmd, resp, 6000);
      String up = j.cmd; up.toUpperCase();
      if (up.startsWith("AT")) currentHeader = "";   // Header evtl. manuell geändert
      r["ok"] = ok;
      r["raw"] = resp;
      if (!ok) r["error"] = T("Kein Prompt (Timeout)", "No prompt (timeout)");
    } else {  // JOB_TEST
      std::vector<uint8_t> bytes;
      String raw;
      bool ok = query(j.header, j.cmd, bytes, raw, err);
      r["raw"] = raw;
      if (ok) {
        r["bytes"] = ObdParse::toHex(bytes).c_str();
        r["count"] = bytes.size();
        if (j.formula.length()) {
          double v;
          std::string e;
          if (ObdParse::evalFormula(j.formula.c_str(), bytes, v, e)) r["value"] = v;
          else { ok = false; err = e.c_str(); }
        }
      }
      r["ok"] = ok;
      if (!ok) r["error"] = err;
    }
  }
  String out;
  serializeJson(r, out);
  lastJobMs = millis();
  std::lock_guard<std::mutex> lk(mtx);
  job.resultJson = out;
  job.state = "done";
}

bool submitJob(JobType t, const String& header, const String& cmd, const String& formula) {
  std::lock_guard<std::mutex> lk(mtx);
  if (job.state == "pending" || job.state == "running") return false;
  job.type = t;
  job.header = header; job.header.toUpperCase();
  job.cmd = cmd; job.cmd.trim();
  if (t != JOB_RAW) job.cmd.toUpperCase();
  job.formula = formula;
  job.state = "pending";
  job.resultJson = "";
  return true;
}

void jobJson(JsonDocument& d) {
  std::lock_guard<std::mutex> lk(mtx);
  d["state"] = job.state;
  if (job.resultJson.length()) {
    JsonDocument r;
    deserializeJson(r, job.resultJson);
    d["result"] = r;
  }
}

// ---------------------------------------------------------------

void requestConfigApply(const String& json) {
  std::lock_guard<std::mutex> lk(mtx);
  pendingConfig = json;
}
void requestProfileReload() { flagProfileReload = true; }
void requestPollNow() { flagPollNow = true; }

static void applyPendingConfig() {
  String js;
  {
    std::lock_guard<std::mutex> lk(mtx);
    if (pendingConfig.isEmpty()) return;
    js = pendingConfig;
    pendingConfig = "";
  }
  AppConfig before = cfg;
  JsonDocument d;
  if (deserializeJson(d, js)) return;
  {
    std::lock_guard<std::mutex> lk(mtx);
    configFromJson(d.as<JsonVariantConst>());
  }
  saveConfig();
  logSetPersist(cfg.logPersist);
  logf("%s", T("Konfiguration gespeichert", "Configuration saved"));
  if (before.bleMac != cfg.bleMac || before.bleService != cfg.bleService ||
      before.bleNotify != cfg.bleNotify || before.bleWrite != cfg.bleWrite ||
      before.bleAddrType != cfg.bleAddrType) {
    ElmBle::disconnect();
    elmReady = false;
    nextConnectTry = 0;
  }
  if (before.profile != cfg.profile) reloadProfile();
  if (before.deviceName != cfg.deviceName || before.lang != cfg.lang) MqttHa::requestDiscovery();
  // WLAN/MQTT-Änderungen werden durch Neustart aus der Web-UI wirksam
}

static bool bleOn = true;
void begin(bool bleEnabled) {
  bleOn = bleEnabled;
  if (bleOn) ElmBle::begin();
  reloadProfile();
}

static void updateCache() {
  bool b = ElmBle::connected();
  int r = b ? ElmBle::rssi() : 0;
  bool m = MqttHa::connected();
  static uint32_t lastInfo = 0;
  bool info = millis() - lastInfo > 2000;   // Texte nur gelegentlich kopieren
  String nm, kd, uu;
  if (info) { lastInfo = millis(); nm = ElmBle::deviceName(); kd = ElmBle::dongleKind(); uu = ElmBle::detectedUuids(); }
  std::lock_guard<std::mutex> lk(mtx);
  cacheBle = b; cacheRssi = r; cacheMqtt = m;
  if (info) { cacheName = nm; cacheKind = kd; cacheUuids = uu; }
}

static volatile bool paused = false;
bool isPaused() { return paused || !bleOn; }
void setPaused(bool p) {
  paused = p;
  if (p) logf("%s", T("Abfragen pausiert (Firmware-Update)", "Polling paused (firmware update)"));
}

void loop() {
  if (!bleOn) { applyPendingConfig(); if (flagProfileReload) { flagProfileReload = false; reloadProfile(); } return; }
  if (paused) { if (ElmBle::connected()) { ElmBle::disconnect(); elmReady = false; } updateCache(); return; }
  applyPendingConfig();
  if (flagProfileReload) { flagProfileReload = false; reloadProfile(); }
  runJob();

  uint32_t now = millis();
  if (flagPollNow) {
    flagPollNow = false;
    std::lock_guard<std::mutex> lk(mtx);
    for (auto& s : states) s.lastPoll = 0;
    nextConnectTry = 0;
  }
  if (cfg.bleMac.isEmpty()) { updateCache(); return; }

  uint32_t wait = msUntilNextDue(now);
  if (wait == 0) {
    if ((int32_t)(now - nextConnectTry) >= 0) pollDue();
  }
  if (ElmBle::connected() && !cfg.keepConnected && msUntilNextDue(millis()) > 20000) {
    bool jobBusy;
    {
      std::lock_guard<std::mutex> lk(mtx);
      jobBusy = job.state == "pending" || job.state == "running";
    }
    if (!jobBusy && (!lastJobMs || millis() - lastJobMs > 60000)) {
      ElmBle::disconnect();   // Dongle darf schlafen
      elmReady = false;
    }
  }
  updateCache();
}

void statusJson(JsonDocument& d) {
  std::lock_guard<std::mutex> lk(mtx);
  uint32_t now = millis();
  d["fw"] = FW_VERSION;
  d["uptime"] = now / 1000;
  d["heap"] = ESP.getFreeHeap();
  d["wifi"]["connected"] = WiFi.status() == WL_CONNECTED;
  d["wifi"]["ssid"] = WiFi.SSID();
  d["wifi"]["ip"] = WiFi.localIP().toString();
  d["wifi"]["rssi"] = WiFi.RSSI();
  d["mqtt"] = cacheMqtt;
  d["ble"]["connected"] = cacheBle;
  d["ble"]["mac"] = cfg.bleMac;
  d["ble"]["rssi"] = cacheRssi;
  d["ble"]["uuids"] = cacheUuids;
  d["ble"]["name"] = cacheName;
  d["ble"]["kind"] = cacheKind;
  d["ble"]["elm"] = elmVersion;
  if (timeValid()) {
    time_t tnow = time(nullptr);
    struct tm tmv;
    localtime_r(&tnow, &tmv);
    char tb[32];
    strftime(tb, sizeof(tb), "%d.%m.%Y %H:%M:%S %Z", &tmv);
    d["time"] = tb;
  }
  d["ble"]["last_ok_s"] = lastConnectOk ? (int32_t)((now - lastConnectOk) / 1000) : -1;
  d["ble"]["next_try_s"] = (int32_t)(nextConnectTry - now) > 0 ? (int32_t)((nextConnectTry - now) / 1000) : 0;
  if (!isnan(lastVoltage)) d["voltage"] = lastVoltage;
  d["profile"] = cfg.profile;
  d["profile_name"] = profile.name;
  d["last_error"] = lastError;
  d["cycles"] = pollCycles;
  JsonArray arr = d["pids"].to<JsonArray>();
  for (size_t i = 0; i < profile.pids.size() && i < states.size(); i++) {
    const PidDef& p = profile.pids[i];
    const PidState& s = states[i];
    JsonObject o = arr.add<JsonObject>();
    o["id"] = p.id; o["name"] = p.name; o["unit"] = p.unit; o["enabled"] = p.enabled;
    if (!isnan(s.value)) o["value"] = serialized(String(s.value, p.precision));
    o["age_s"] = s.lastOk ? (int32_t)((now - s.lastOk) / 1000) : -1;
    if (s.lastPoll) {
      uint32_t el = now - s.lastPoll, iv = p.interval * 1000UL;
      o["next_s"] = el >= iv ? 0 : (iv - el) / 1000;
    } else o["next_s"] = 0;
    o["error"] = s.err;
  }
  JsonArray lg = d["log"].to<JsonArray>();
  logToJson(lg);
}

}  // namespace Poller
