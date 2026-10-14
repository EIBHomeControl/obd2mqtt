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

// Release-Text nur bis zum Abschnitt der installierten Version (ältere Änderungen kennt man schon)
static String newerNotes(const String& body) {
  String cur = FW_VERSION;
  int from = 0;
  while (from < (int)body.length()) {
    int nl = body.indexOf('\n', from);
    if (nl < 0) nl = body.length();
    String line = body.substring(from, nl);
    if (line.startsWith("##") && from > 0) {
      int p = line.indexOf(cur);
      if (p >= 0) {
        char after = p + cur.length() < line.length() ? line[p + cur.length()] : ' ';
        if (!isdigit((unsigned char)after)) return body.substring(0, from);
      }
    }
    from = nl + 1;
  }
  return body;
}

static void connectTls(WiFiClientSecure& c) {
  c.setCACert(GH_ROOT_CAS);
  c.setTimeout(15);   // Sekunden
}

static void doCheck() {
  if (WiFi.status() != WL_CONNECTED) { set("error", T("kein WLAN", "no WiFi")); return; }
  set("checking");
  Watchdog::feed();
  Watchdog::step("update check");
  logf("%s", T("Online-Update: prüfe GitHub auf neue Firmware…", "Online update: checking GitHub for new firmware…"));
  WiFiClientSecure cli;
  connectTls(cli);
  HTTPClient http;
  http.useHTTP10(true);   // kein „chunked“ – Antwort lässt sich direkt als Stream parsen
  http.setTimeout(15000);
  http.setUserAgent("obd2mqtt/" FW_VERSION);
  String err;
  if (!http.begin(cli, "https://api.github.com/repos/" UPDATE_REPO "/releases/latest")) {
    set("error", T("Verbindung zu GitHub fehlgeschlagen", "Connecting to GitHub failed"));
    Watchdog::step("Poller");
    return;
  }
  http.addHeader("Accept", "application/vnd.github+json");
  int code = http.GET();
  Watchdog::feed();
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
  filter["body"] = true;
  filter["assets"][0]["name"] = true;
  filter["assets"][0]["size"] = true;
  filter["assets"][0]["browser_download_url"] = true;
  JsonDocument d;
  DeserializationError e = deserializeJson(d, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  Watchdog::feed();
  Watchdog::step("Poller");
  if (e) { set("error", String(T("Antwort von GitHub unlesbar: ", "Unreadable response from GitHub: ")) + e.c_str()); return; }

  String tag = d["tag_name"] | "";
  String url, body = d["body"] | "";
  uint32_t size = 0;
  for (JsonObject a : d["assets"].as<JsonArray>()) {
    String n = a["name"] | "";
    if (n.endsWith("-esp32-ota.bin")) { url = a["browser_download_url"] | ""; size = a["size"] | 0; break; }
  }
  body.replace("\r", "");
  String nn = newerNotes(body);
  if (nn.length() > 3000) nn = nn.substring(0, 3000) + "\n…";
  bool newer = tag.length() && cmpVer(tag, FW_VERSION) > 0;
  char ts[24] = "";
  if (timeValid()) { time_t t = time(nullptr); struct tm tmv; localtime_r(&t, &tmv); strftime(ts, sizeof(ts), "%d.%m.%Y %H:%M", &tmv); }
  {
    std::lock_guard<std::mutex> lk(mtx);
    latest = tag;
    releaseUrl = d["html_url"] | "";
    notes = newer ? nn : String("");
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
  bool bleOff = false;
  if (ESP.getMaxAllocHeap() < 60000) { ElmBle::shutdown(); bleOff = true; delay(200); }
  Watchdog::feed();

  WiFiClientSecure cli;
  connectTls(cli);
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(20000);
  http.setUserAgent("obd2mqtt/" FW_VERSION);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);   // github.com → release-assets.githubusercontent.com
  String err;
  int code = 0;
  if (!http.begin(cli, url)) err = T("Verbindung zu GitHub fehlgeschlagen", "Connecting to GitHub failed");
  else {
    code = http.GET();
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
  bool autoDue = cfg.updateCheck && WiFi.status() == WL_CONNECTED &&
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
    if (notes.length()) d["release_summary"] = notes.substring(0, 240);
  }
  String s;
  serializeJson(d, s);
  MqttHa::publishText("update", s);
}

}  // namespace OnlineUpdate
