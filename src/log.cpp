#include "log.h"
#include <LittleFS.h>
#include <deque>
#include <mutex>
#include <time.h>

static std::mutex mtx;
static std::deque<String> lines;      // RAM-Ringpuffer
static std::deque<String> pending;    // noch nicht in die Datei geschrieben
static const size_t MAX_LINES = 200;
static const size_t MAX_FILE = 48 * 1024;
static bool persist = true;
static const char* LOG_FILE = "/log.txt";
static const char* LOG_OLD = "/log.old.txt";

bool timeValid() { return time(nullptr) > 1700000000; }

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

void logFlush() {
  std::deque<String> todo;
  {
    std::lock_guard<std::mutex> lk(mtx);
    if (!persist || pending.empty()) return;
    todo.swap(pending);
  }
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

void logClear() {
  {
    std::lock_guard<std::mutex> lk(mtx);
    lines.clear();
    pending.clear();
  }
  LittleFS.remove(LOG_FILE);
  LittleFS.remove(LOG_OLD);
}
