#include "log.h"
#include <LittleFS.h>
#include <deque>
#include <mutex>
#include <time.h>
#include "i18n.h"

static std::mutex mtx;
static std::mutex fmtx;               // serialisiert alle Zugriffe auf die Log-Dateien (loop- und Web-Task)
static std::deque<String> lines;      // RAM-Ringpuffer
static std::deque<String> pending;    // noch nicht in die Datei geschrieben
static const size_t MAX_LINES = 200;
static const size_t MAX_FILE = 16 * 1024;     // je Datei; zusammen max. ~32 KB von 128 KB LittleFS
static const float FS_MAX_USE = 0.70f;          // darüber werden Log-Dateien geopfert, nie Profile/Config
static bool persist = true;
static const char* LOG_FILE = "/log.txt";
static const char* LOG_OLD = "/log.old.txt";

// ---------- Uhrzeit ----------
// Die Zeit gilt erst als gültig, wenn sie plausibel ist (nicht vor dem Firmware-Bau, höchstens 5 Jahre danach)
// oder von einer zweiten NTP-Antwort bestätigt wurde. Ein fehlerhafter NTP-Server lieferte z. B. 2034.
#include <esp_sntp.h>
static volatile bool gTimeOk = false;
static volatile bool gSyncEvent = false;
static time_t gBuild = 0;
static time_t gCand = 0;          // unplausible Zeit, wartet auf Bestätigung
static uint32_t gCandMs = 0, gLastRestart = 0;

static time_t buildEpoch() {
  static const char* mon = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char m[4] = {0};
  int d = 1, y = 2026;
  sscanf(__DATE__, "%3s %d %d", m, &d, &y);
  struct tm t = {};
  t.tm_year = y - 1900;
  t.tm_mon = (int)((strstr(mon, m) - mon) / 3);
  t.tm_mday = d;
  t.tm_hour = 12;
  setenv("TZ", "UTC0", 1); tzset();
  time_t r = mktime(&t);
  return r;
}
static bool plausible(time_t t) { return t >= gBuild - 86400 && t <= gBuild + 5L * 365 * 86400; }
static void onSync(struct timeval*) { gSyncEvent = true; }

void timeInit(const char* tz) {
  gBuild = buildEpoch();
  setenv("TZ", tz, 1); tzset();
  gTimeOk = plausible(time(nullptr));   // nach Software-Neustart läuft die Uhr weiter
  sntp_set_time_sync_notification_cb(onSync);
}

static String fmtTime(time_t t) {
  struct tm tmv; localtime_r(&t, &tmv);
  char b[24]; strftime(b, sizeof(b), "%Y-%m-%d %H:%M:%S", &tmv);
  return b;
}

void timeLoop(const char* server) {
  if (!gSyncEvent) {
    // Unplausible Zeit gesetzt und noch keine Bestätigung: jede Minute neu anfragen
    if (!gTimeOk && gCand && millis() - gLastRestart > 60000) { gLastRestart = millis(); sntp_restart(); }
    return;
  }
  gSyncEvent = false;
  time_t t = time(nullptr);
  bool was = gTimeOk;
  if (plausible(t)) {
    gTimeOk = true;
    gCand = 0;
  } else if (gCand && labs((long)((t - gCand) - (time_t)((millis() - gCandMs) / 1000))) < 300) {
    gTimeOk = true;   // zweite Antwort bestätigt die (ungewöhnliche) Zeit
    gCand = 0;
  } else {
    gTimeOk = false;
    gCand = t; gCandMs = millis(); gLastRestart = millis();
    logf("NTP: %s %s – %s", T("unplausible Uhrzeit", "implausible time"), fmtTime(t).c_str(),
         T("verworfen, frage erneut", "discarded, asking again"));
    sntp_restart();
    return;
  }
  if (!was && gTimeOk) logf(T("Uhrzeit per NTP synchronisiert (%s)", "Time synchronized via NTP (%s)"), server);
}

bool timeValid() { return gTimeOk; }

static void stamp(char* ts, size_t n) {
  if (timeValid()) {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    strftime(ts, n, "%Y-%m-%d %H:%M:%S ", &t);
  } else {   // vor der NTP-Synchronisation: Laufzeit seit Start
    uint32_t s = millis() / 1000;
    snprintf(ts, n, "+%02lu:%02lu:%02lu ", (unsigned long)(s / 3600) % 100,
             (unsigned long)(s / 60) % 60, (unsigned long)s % 60);
  }
}

void logf(const char* fmt, ...) {
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  char ts[24];
  stamp(ts, sizeof(ts));
  String line = String(ts) + buf;
  Serial.println(line);
  std::lock_guard<std::mutex> lk(mtx);
  lines.push_back(line);
  while (lines.size() > MAX_LINES) lines.pop_front();
  if (persist) {
    pending.push_back(line);
    while (pending.size() > MAX_LINES) pending.pop_front();
  }
}

void logToJson(JsonArray arr, size_t maxLines) {
  std::lock_guard<std::mutex> lk(mtx);
  size_t start = lines.size() > maxLines ? lines.size() - maxLines : 0;
  for (size_t i = start; i < lines.size(); i++) arr.add(lines[i]);
}

String logText() {
  std::lock_guard<std::mutex> lk(mtx);
  String s;
  s.reserve(lines.size() * 70);
  for (auto& l : lines) { s += l; s += '\n'; }
  return s;
}

void logSetPersist(bool on) {
  std::lock_guard<std::mutex> lk(mtx);
  persist = on;
  if (!on) pending.clear();
}
bool logPersist() { return persist; }

// Platz schaffen: zuerst das alte Log, notfalls auch das aktuelle löschen
static void ensureSpace(size_t need) {
  size_t total = LittleFS.totalBytes();
  if (!total) return;
  auto tooFull = [&]() { return LittleFS.usedBytes() + need + 8192 > (size_t)(total * FS_MAX_USE); };
  if (tooFull() && LittleFS.exists(LOG_OLD)) LittleFS.remove(LOG_OLD);
  if (tooFull() && LittleFS.exists(LOG_FILE)) LittleFS.remove(LOG_FILE);
}

void logFlush() {
  std::deque<String> todo;
  {
    std::lock_guard<std::mutex> lk(mtx);
    if (!persist || pending.empty()) return;
    todo.swap(pending);
  }
  size_t need = 0;
  for (auto& l : todo) need += l.length() + 1;
  std::lock_guard<std::mutex> fl(fmtx);
  ensureSpace(need);
  if (LittleFS.totalBytes() && LittleFS.usedBytes() + need + 8192 > (size_t)(LittleFS.totalBytes() * FS_MAX_USE))
    return;   // trotzdem zu voll (Profile sehr groß) → lieber nichts schreiben als abstürzen
  File f = LittleFS.open(LOG_FILE, "a");
  if (!f) return;
  for (auto& l : todo) { f.print(l); f.print('\n'); }
  size_t sz = f.size();
  f.close();
  if (sz > MAX_FILE) {   // rotieren: aktuelle Datei wird zur alten
    LittleFS.remove(LOG_OLD);
    LittleFS.rename(LOG_FILE, LOG_OLD);
  }
}

bool logReadFile(bool old, String& out) {
  std::lock_guard<std::mutex> fl(fmtx);
  const char* path = old ? LOG_OLD : LOG_FILE;
  if (!LittleFS.exists(path)) return false;
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  size_t sz = f.size();
  if (sz > MAX_FILE + 4096) { f.seek(sz - (MAX_FILE + 4096)); sz = MAX_FILE + 4096; }   // Altlasten > 16 KB: nur das Ende
  out.reserve(sz + 1);
  uint8_t buf[512];
  while (f.available()) {
    int n = f.read(buf, sizeof(buf));
    if (n <= 0) break;
    out.concat((const char*)buf, n);
  }
  f.close();
  return true;
}

bool logFileExists(bool old) {
  std::lock_guard<std::mutex> fl(fmtx);
  return LittleFS.exists(old ? LOG_OLD : LOG_FILE);
}

size_t logReadChunk(bool old, size_t offset, uint8_t* buf, size_t maxLen) {
  std::lock_guard<std::mutex> fl(fmtx);
  const char* path = old ? LOG_OLD : LOG_FILE;
  if (!LittleFS.exists(path)) return 0;
  File f = LittleFS.open(path, "r");
  if (!f) return 0;
  size_t n = 0;
  if (offset < f.size() && f.seek(offset)) n = f.read(buf, min(maxLen, (size_t)1024));
  f.close();
  return n;
}

void logBootCheck(uint32_t crashes) {
  std::lock_guard<std::mutex> fl(fmtx);
  size_t total = LittleFS.totalBytes(), used = LittleFS.usedBytes();
  bool full = total && used > (size_t)(total * FS_MAX_USE);
  // Absturzschleife oder volles Dateisystem: Log-Dateien sind der wahrscheinlichste Auslöser → weg damit
  if (crashes >= 2 || full) LittleFS.remove(LOG_OLD);
  if (crashes >= 3 || (full && LittleFS.usedBytes() > (size_t)(total * FS_MAX_USE))) LittleFS.remove(LOG_FILE);
  {
    // Altlast aus Versionen ≤ 0.3.5 (48 KB je Datei) kürzen
    for (const char* p : {LOG_OLD, LOG_FILE}) {
      if (!LittleFS.exists(p)) continue;
      File f = LittleFS.open(p, "r");
      if (!f) continue;
      size_t sz = f.size();
      f.close();
      if (sz > MAX_FILE + 4096) LittleFS.remove(p);
    }
  }
}

void logClear() {
  {
    std::lock_guard<std::mutex> lk(mtx);
    lines.clear();
    pending.clear();
  }
  std::lock_guard<std::mutex> fl(fmtx);
  LittleFS.remove(LOG_FILE);
  LittleFS.remove(LOG_OLD);
}
