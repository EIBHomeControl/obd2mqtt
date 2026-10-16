#include "online_update.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <mutex>
#include "config.h"
#include "elm_ble.h"
#include "gh_roots.h"
#include "i18n.h"
#include "log.h"
#include "mqtt_ha.h"
#include "poller.h"
#include "watchdog.h"

#ifndef UPDATE_REPO
#define UPDATE_REPO "EIBHomeControl/obd2mqtt"
#endif

namespace OnlineUpdate {

static std::mutex mtx;
static String state = "idle";          // idle, checking, uptodate, available, error, downloading, done
static String latest, notes, assetUrl, releaseUrl, error, checkedAt;
static uint32_t assetSize = 0, written = 0;
static volatile bool wantCheck = false, wantInstall = false;
static uint32_t lastCheckMs = 0;
static String announced;               // für diese Version schon „verfügbar“ geloggt

static void set(const String& st, const String& err = "") {
  std::lock_guard<std::mutex> lk(mtx);
  state = st;
  error = err;
}

// "v0.3.22" / "0.3.22" → vergleichbar; >0 wenn a neuer als b
static int cmpVer(String a, String b) {
  a.trim(); b.trim();
  if (a.startsWith("v") || a.startsWith("V")) a.remove(0, 1);
  if (b.startsWith("v") || b.startsWith("V")) b.remove(0, 1);
  for (int i = 0; i < 4; i++) {
    int pa = a.indexOf('.'), pb = b.indexOf('.');
    long x = a.toInt(), y = b.toInt();
    if (x != y) return x > y ? 1 : -1;
    if (pa < 0 && pb < 0) return 0;
    a = pa < 0 ? String("0") : a.substring(pa + 1);
    b = pb < 0 ? String("0") : b.substring(pb + 1);
  }
  return 0;
}

static void connectTls(WiFiClientSecure& c) {
  c.setCACert(GH_ROOT_CAS);
  c.setTimeout(15);              // Sekunden je Lese-/Schreibvorgang
  c.setHandshakeTimeout(20);     // Standard wären 120 s – länger als der 60-s-Watchdog
}

static String kb(uint32_t b) { return String(b / 1024) + " KB"; }

// Eigener TLS-Verbindungsaufbau mit Zeitmessung und Klartext-Fehler (HTTPClient nutzt die Verbindung danach weiter)
static bool tlsConnect(WiFiClientSecure& c, const char* host, String& err) {
  logf(T("Online-Update: verbinde mit %s (Heap %s, größter Block %s)", "Online update: connecting to %s (heap %s, largest block %s)"),
       host, kb(ESP.getFreeHeap()).c_str(), kb(ESP.getMaxAllocHeap()).c_str());
  logFlush();   // falls die TLS-Bibliothek abstürzt, stehen die Zeilen davor schon im Flash
  Watchdog::feed();
  Watchdog::step("update TLS");
  uint32_t t0 = millis();
  bool ok = c.connect(host, 443, 15000);
  uint32_t dt = millis() - t0;
  Watchdog::feed();
  if (!ok) {
    char e[100] = "";
    int code = c.lastError(e, sizeof(e));
    err = String(T("TLS-Verbindung zu ", "TLS connection to ")) + host + T(" fehlgeschlagen nach ", " failed after ") + dt + " ms" +
          (code ? String(" (") + code + ": " + e + ")" : String(""));
    logf(T("Online-Update: %s (Heap danach %s)", "Online update: %s (heap afterwards %s)"), err.c_str(), kb(ESP.getFreeHeap()).c_str());
    return false;
  }
  logf(T("Online-Update: TLS verbunden nach %lu ms (Heap %s)", "Online update: TLS connected after %lu ms (heap %s)"),
       (unsigned long)dt, kb(ESP.getFreeHeap()).c_str());
  return true;
}

// Liest die Antwort gepuffert, füttert dabei den Watchdog und bricht nach maxMs ab
// (ArduinoJson liest sonst Zeichen für Zeichen direkt vom TLS-Client, ohne Gesamt-Zeitlimit).
class GuardStream : public Stream {
 public:
  GuardStream(Client& c, uint32_t maxMs) : c_(c), t0_(millis()), max_(maxMs) {}
  int available() override { return pos_ < len_ ? len_ - pos_ : c_.available(); }
  int read() override { return (pos_ < len_ || fill()) ? buf_[pos_++] : -1; }
  int peek() override { return (pos_ < len_ || fill()) ? buf_[pos_] : -1; }
  size_t readBytes(char* b, size_t n) override {
    size_t k = 0;
    while (k < n) { int ch = read(); if (ch < 0) break; b[k++] = (char)ch; }
    return k;
  }
  size_t write(uint8_t) override { return 0; }
  void flush() override {}
  size_t total = 0;
  bool timedOut = false, closed = false;
 private:
  bool fill() {
    while (true) {
      Watchdog::feed();
      if (millis() - t0_ > max_) { timedOut = true; return false; }
      int a = c_.available();
      if (a > 0) {
        int n = c_.read(buf_, a < (int)sizeof(buf_) ? a : (int)sizeof(buf_));
        if (n > 0) { pos_ = 0; len_ = n; total += n; return true; }
      } else if (!c_.connected()) { closed = true; return false; }
      delay(2);
    }
  }
  Client& c_;
  uint32_t t0_, max_;
  uint8_t buf_[512];
  int pos_ = 0, len_ = 0;
};

// Vor dem TLS-Aufbau: reicht der Speicher? Sonst klarer Abbruch statt Absturz in der TLS-Bibliothek.
static bool enoughMemory() {
  uint32_t fh = ESP.getFreeHeap(), mb = ESP.getMaxAllocHeap();
  if (fh < 60000 || mb < 40000) {
    String m = String(T("zu wenig freier Speicher für die verschlüsselte Verbindung (frei ", "not enough free memory for the encrypted connection (free ")) +
               kb(fh) + T(", größter Block ", ", largest block ") + kb(mb) + T(", nötig ca. 60 / 40 KB)", ", needed approx. 60 / 40 KB)");
    set("error", m);
    logf(T("Online-Update: %s", "Online update: %s"), m.c_str());
    return false;
  }
  return true;
}

static void logStack(const char* where) {
  logf(T("Online-Update: %s – Heap %s (min. %s), freier Stack %u Bytes", "Online update: %s – heap %s (min. %s), free stack %u bytes"),
       where, kb(ESP.getFreeHeap()).c_str(), kb(ESP.getMinFreeHeap()).c_str(), (unsigned)uxTaskGetStackHighWaterMark(nullptr));
}

static void doCheck() {
  if (WiFi.status() != WL_CONNECTED) { set("error", T("kein WLAN", "no WiFi")); return; }
  set("checking");
  Watchdog::feed();
  Watchdog::step("update check");
  logf("%s", T("Online-Update: prüfe GitHub auf neue Firmware…", "Online update: checking GitHub for new firmware…"));
  // Speicher für TLS (~45 KB) freimachen: Abfragen kurz pausieren, Bluetooth-Verbindung trennen
  bool wasPaused = Poller::isPaused();
  if (!wasPaused) { Poller::setPausedQuiet(true); ElmBle::disconnect(); delay(300); }
  struct Resume { bool p; ~Resume() { if (!p) Poller::setPausedQuiet(false); } } resume{wasPaused};
  if (!enoughMemory()) { Watchdog::step("Poller"); return; }
  WiFiClientSecure cli;
  connectTls(cli);
  String err;
  if (!tlsConnect(cli, "api.github.com", err)) { set("error", err); Watchdog::step("Poller"); return; }
  HTTPClient http;
  http.useHTTP10(true);   // kein „chunked“ – Antwort lässt sich direkt als Stream parsen
  http.setTimeout(15000);
  http.setUserAgent("obd2mqtt/" FW_VERSION);
  http.setReuse(true);
  if (!http.begin(cli, "https://api.github.com/repos/" UPDATE_REPO "/releases/latest")) {
    set("error", T("Verbindung zu GitHub fehlgeschlagen", "Connecting to GitHub failed"));
    Watchdog::step("Poller");
    return;
  }
  http.addHeader("Accept", "application/vnd.github+json");
  Watchdog::step("update GET");
  uint32_t t0 = millis();
  int code = http.GET();
  Watchdog::feed();
  logf(T("Online-Update: Antwort HTTP %d nach %lu ms (Länge %d)", "Online update: response HTTP %d after %lu ms (length %d)"),
       code, (unsigned long)(millis() - t0), http.getSize());
  if (code != 200) {
    err = code == 404 ? String(T("noch kein Release veröffentlicht", "no release published yet"))
        : code < 0 ? String(T("GitHub nicht erreichbar (", "GitHub not reachable (")) + http.errorToString(code) + ")"
                   : String("HTTP ") + code + (code == 403 ? T(" (Abfragelimit von GitHub – später erneut versuchen)", " (GitHub rate limit – try again later)") : "");
    http.end();
    set("error", err);
    logf(T("Online-Update: %s", "Online update: %s"), err.c_str());
    Watchdog::step("Poller");
    return;
  }
  JsonDocument filter;
  filter["tag_name"] = true;
  filter["html_url"] = true;
  // "body" (Änderungstext) bewusst nicht: kostet bei offener TLS-Verbindung zu viel RAM („NoMemory“).
  // Die Weboberfläche holt den Text selbst im Browser von GitHub.
  filter["assets"][0]["name"] = true;
  filter["assets"][0]["size"] = true;
  filter["assets"][0]["browser_download_url"] = true;
  JsonDocument d;
  Watchdog::step("update parse");
  t0 = millis();
  GuardStream gs(cli, 30000);
  DeserializationError e = deserializeJson(d, gs, DeserializationOption::Filter(filter));
  logf(T("Online-Update: %u Bytes gelesen in %lu ms%s%s (Heap %s)", "Online update: read %u bytes in %lu ms%s%s (heap %s)"),
       (unsigned)gs.total, (unsigned long)(millis() - t0), gs.timedOut ? T(", Zeitlimit erreicht", ", time limit reached") : "",
       gs.closed ? T(", Verbindung beendet", ", connection closed") : "", kb(ESP.getFreeHeap()).c_str());
  http.end();
  cli.stop();
  logStack(T("Prüfung beendet", "check finished"));
  Watchdog::feed();
  Watchdog::step("Poller");
  if (e) {
    String m = String(T("Antwort von GitHub unlesbar: ", "Unreadable response from GitHub: ")) + e.c_str();
    if (gs.timedOut) m += T(" (Zeitlimit 30 s)", " (time limit 30 s)");
    set("error", m);
    logf(T("Online-Update: %s", "Online update: %s"), m.c_str());
    return;
  }

  String tag = d["tag_name"] | "";
  String url;
  uint32_t size = 0;
  for (JsonObject a : d["assets"].as<JsonArray>()) {
    String n = a["name"] | "";
    if (n.endsWith("-esp32-ota.bin")) { url = a["browser_download_url"] | ""; size = a["size"] | 0; break; }
  }
  bool newer = tag.length() && cmpVer(tag, FW_VERSION) > 0;
  char ts[24] = "";
  if (timeValid()) { time_t t = time(nullptr); struct tm tmv; localtime_r(&t, &tmv); strftime(ts, sizeof(ts), "%d.%m.%Y %H:%M", &tmv); }
  {
    std::lock_guard<std::mutex> lk(mtx);
    latest = tag;
    releaseUrl = d["html_url"] | "";
    notes = "";
    assetUrl = url;
    assetSize = size;
    checkedAt = ts;
    if (!newer) { state = "uptodate"; error = ""; }
    else if (url.isEmpty()) { state = "error"; error = String(T("Release ", "Release ")) + tag + T(" enthält keine *-esp32-ota.bin", " contains no *-esp32-ota.bin"); }
    else { state = "available"; error = ""; }
  }
  if (newer && announced != tag) {
    announced = tag;
    logf(T("Neue Firmware verfügbar: %s (installiert: %s) – System → Online-Update", "New firmware available: %s (installed: %s) – System → Online update"), tag.c_str(), FW_VERSION);
  } else if (!newer) logf(T("Online-Update: Firmware ist aktuell (%s)", "Online update: firmware is up to date (%s)"), FW_VERSION);
  publishMqtt();
}

static void doInstall() {
  String url, tag;
  uint32_t size;
  {
    std::lock_guard<std::mutex> lk(mtx);
    url = assetUrl; tag = latest; size = assetSize;
  }
  if (url.isEmpty()) { set("error", T("Zuerst auf Updates prüfen", "Check for updates first")); return; }
  set("downloading");
  written = 0;
  logf(T("Online-Update auf %s gestartet", "Online update to %s started"), tag.c_str());
  Poller::setPaused(true);                  // keine BLE-Abfragen mehr
  Watchdog::step("update download");
  // TLS + Download brauchen ~45 KB am Stück – Bluetooth abschalten, es folgt ohnehin ein Neustart
  bool bleOff = true;
  {
    ElmBle::shutdown(); delay(300);   // Neustart folgt ohnehin – Bluetooth-Speicher für TLS + Download nutzen
    logf(T("Online-Update: Bluetooth abgeschaltet für mehr Speicher (Heap jetzt %s, größter Block %s)", "Online update: Bluetooth switched off for more memory (heap now %s, largest block %s)"),
         kb(ESP.getFreeHeap()).c_str(), kb(ESP.getMaxAllocHeap()).c_str());
  }
  Watchdog::feed();
  if (!enoughMemory()) { Watchdog::restart(T("Online-Update: zu wenig Speicher", "Online update: not enough memory")); return; }

  WiFiClientSecure cli;
  connectTls(cli);
  logf(T("Online-Update: lade %s (%s)", "Online update: downloading %s (%s)"), url.c_str(), kb(size).c_str());
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(20000);
  http.setUserAgent("obd2mqtt/" FW_VERSION);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);   // github.com → release-assets.githubusercontent.com
  String err;
  int code = 0;
  if (!http.begin(cli, url)) err = T("Verbindung zu GitHub fehlgeschlagen", "Connecting to GitHub failed");
  else {
    Watchdog::step("update GET bin");
    uint32_t t0 = millis();
    code = http.GET();
    Watchdog::feed();
    logf(T("Online-Update: Download-Antwort HTTP %d nach %lu ms (Länge %d, Heap %s)", "Online update: download response HTTP %d after %lu ms (length %d, heap %s)"),
         code, (unsigned long)(millis() - t0), http.getSize(), kb(ESP.getFreeHeap()).c_str());
    if (code < 0) {
      char e[100] = "";
      if (cli.lastError(e, sizeof(e))) logf(T("Online-Update: TLS-Fehler: %s", "Online update: TLS error: %s"), e);
    }
    Watchdog::step("update download");
    if (code != 200) err = String(T("Download fehlgeschlagen: ", "Download failed: ")) + (code < 0 ? http.errorToString(code) : String("HTTP ") + code);
  }
  int len = err.isEmpty() ? http.getSize() : 0;
  if (err.isEmpty() && (len <= 0 || (size && (uint32_t)len != size))) err = T("Dateigröße stimmt nicht", "File size mismatch");
  if (err.isEmpty() && !Update.begin(len, U_FLASH)) err = String(T("Update nicht möglich: ", "Update not possible: ")) + Update.errorString();
  if (err.isEmpty()) {
    WiFiClient* s = http.getStreamPtr();
    uint8_t buf[1024];
    uint32_t lastData = millis();
    while (written < (uint32_t)len) {
      Watchdog::feed();
      size_t avail = s->available();
      if (!avail) {
        if (!http.connected() || millis() - lastData > 20000) { err = T("Download abgebrochen", "Download aborted"); break; }
        delay(5);
        continue;
      }
      int n = s->readBytes(buf, min(avail, sizeof(buf)));
      if (n <= 0) continue;
      lastData = millis();
      if (Update.write(buf, n) != (size_t)n) { err = String(T("Schreibfehler: ", "Write error: ")) + Update.errorString(); break; }
      written += n;
    }
    if (err.isEmpty() && !Update.end(true)) err = String(T("Prüfung fehlgeschlagen: ", "Verification failed: ")) + Update.errorString();
    if (!err.isEmpty() && Update.isRunning()) Update.abort();
  }
  http.end();
  if (!err.isEmpty()) {
    set("error", err);
    logf(T("Online-Update fehlgeschlagen: %s", "Online update failed: %s"), err.c_str());
    // Bluetooth evtl. abgeschaltet → sauber neu starten statt halb lauffähig weiterzumachen
    if (!bleOff) Poller::setPaused(false);
    else Watchdog::restart(T("nach fehlgeschlagenem Online-Update", "after failed online update"));
    return;
  }
  set("done");
  logf(T("Online-Update erfolgreich (%s) – Neustart", "Online update successful (%s) – restarting"), tag.c_str());
  for (int i = 0; i < 15; i++) { Watchdog::feed(); delay(100); }   // Web-Oberfläche kann „fertig“ noch abholen
  Watchdog::restart((String(T("Online-Update auf ", "Online update to ")) + tag).c_str());
}

void requestCheck() { wantCheck = true; }

bool requestInstall() {
  std::lock_guard<std::mutex> lk(mtx);
  if (state != "available" || assetUrl.isEmpty()) return false;
  wantInstall = true;
  return true;
}

void loop() {
  if (wantInstall) { wantInstall = false; doInstall(); return; }
  uint32_t now = millis();
  // automatisch: 3 min nach dem Start, danach alle 24 h
  static bool blocked = false, blockLogged = false;
  if (!blockLogged) {
    blockLogged = true;
    String hs = Watchdog::hangStep();
    blocked = Watchdog::safeMode() || hs.startsWith("update");
    if (blocked && cfg.updateCheck)
      logf("%s", T("Online-Update: automatische Prüfung bis zum nächsten Neustart ausgesetzt (letzter Absturz während der Update-Prüfung bzw. sicherer Modus)",
                   "Online update: automatic check suspended until the next restart (last crash during the update check or safe mode)"));
  }
  bool autoDue = !blocked && cfg.updateCheck && WiFi.status() == WL_CONNECTED &&
                 (lastCheckMs == 0 ? now > 180000UL : now - lastCheckMs > 24UL * 3600 * 1000);
  if (wantCheck || autoDue) {
    wantCheck = false;
    lastCheckMs = now ? now : 1;
    doCheck();
  }
}

void stateJson(JsonDocument& d) {
  std::lock_guard<std::mutex> lk(mtx);
  d["state"] = state;
  d["installed"] = FW_VERSION;
  d["latest"] = latest;
  d["notes"] = notes;
  d["url"] = releaseUrl;
  d["error"] = error;
  d["checked"] = checkedAt;
  d["auto"] = cfg.updateCheck;
  if (state == "downloading" && assetSize) { d["written"] = written; d["total"] = assetSize; }
}

void publishMqtt() {
  JsonDocument d;
  {
    std::lock_guard<std::mutex> lk(mtx);
    d["installed_version"] = FW_VERSION;
    d["latest_version"] = latest.length() ? (latest.startsWith("v") ? latest.substring(1) : latest) : String(FW_VERSION);
    d["title"] = "OBD2MQTT";
    if (releaseUrl.length()) d["release_url"] = releaseUrl;
  }
  String s;
  serializeJson(d, s);
  MqttHa::publishText("update", s);
}

}  // namespace OnlineUpdate
