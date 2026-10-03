#include "web.h"
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "log.h"
#include "i18n.h"
#include "poller.h"
#include "web_ui.h"
#include "help_ui.h"
#include "watchdog.h"

namespace Web {

static AsyncWebServer server(80);
static volatile uint32_t rebootAt = 0;
static volatile uint8_t otaState = 0;           // 0 idle, 1 läuft, 2 fertig, 3 Fehler
static volatile size_t otaWritten = 0, otaTotal = 0;
static String otaError;

static bool auth(AsyncWebServerRequest* r) {
  if (cfg.webPass.isEmpty()) return true;
  if (r->authenticate("admin", cfg.webPass.c_str())) return true;
  r->requestAuthentication("OBD2MQTT");
  return false;
}

static void sendJson(AsyncWebServerRequest* r, JsonDocument& d, int code = 200) {
  String s;
  serializeJson(d, s);
  r->send(code, "application/json", s);
}

static void sendMsg(AsyncWebServerRequest* r, bool ok, const String& msg, int code = 0) {
  JsonDocument d;
  d["ok"] = ok;
  d["msg"] = msg;
  sendJson(r, d, code ? code : (ok ? 200 : 400));
}

// Body-Sammler: legt den Request-Body als C-String in _tempObject ab (wird vom Server freigegeben)
static void collectBody(AsyncWebServerRequest* r, uint8_t* data, size_t len, size_t index, size_t total) {
  if (total > 65536) return;   // max. 64 KB (Backup-Datei)
  if (index == 0) {
    r->_tempObject = malloc(total + 1);
    if (!r->_tempObject) return;
  }
  if (!r->_tempObject) return;
  memcpy((uint8_t*)r->_tempObject + index, data, len);
  if (index + len == total) ((char*)r->_tempObject)[total] = 0;
}
static String body(AsyncWebServerRequest* r) {
  return r->_tempObject ? String((const char*)r->_tempObject) : String();
}

static void postJson(const char* path, ArRequestHandlerFunction fn) {
  server.on(path, HTTP_POST, fn, nullptr, collectBody);
}

void begin() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    AsyncWebServerResponse* res = r->beginResponse(200, "text/html", (const uint8_t*)INDEX_HTML, strlen_P(INDEX_HTML));
    res->addHeader("Cache-Control", "no-cache");
    r->send(res);
  });

  // Benutzerhandbuch (liegt im Flash)
  server.on("/hilfe", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    bool en = r->hasParam("lang") ? r->getParam("lang")->value() == "en" : g_lang == 1;
    const char* page = en ? HELP_HTML_EN : HELP_HTML;
    AsyncWebServerResponse* res = r->beginResponse(200, "text/html", (const uint8_t*)page, strlen_P(page));
    res->addHeader("Cache-Control", "no-cache");
    r->send(res);
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    JsonDocument d;
    Poller::statusJson(d);
    d["reset_reason"] = Watchdog::resetReason();
    d["safe_mode"] = Watchdog::safeMode();
    sendJson(r, d);
  });

  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    JsonDocument d;
    {
      std::lock_guard<std::mutex> lk(Poller::mutex());
      configToJson(d, true);
    }
    sendJson(r, d);
  });

  postJson("/api/config", [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    String b = body(r);
    JsonDocument d;
    if (b.isEmpty() || deserializeJson(d, b)) return sendMsg(r, false, T("Ungültiges JSON", "Invalid JSON"));
    Poller::requestConfigApply(b);
    if (r->hasParam("reboot")) rebootAt = millis() + 1500;
    sendMsg(r, true, r->hasParam("reboot") ? T("Gespeichert – Neustart…", "Saved – restarting…") : T("Gespeichert", "Saved"));
  });

  server.on("/api/profiles", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    JsonDocument d;
    {
      std::lock_guard<std::mutex> lk(Poller::mutex());
      d["active"] = cfg.profile;
    }
    JsonArray a = d["list"].to<JsonArray>();
    JsonObject names = d["names"].to<JsonObject>();
    JsonDocument filter;
    filter["name"] = true;
    for (auto& n : listProfiles()) {
      a.add(n);
      JsonDocument p;   // nur den Anzeigenamen lesen
      if (!deserializeJson(p, readProfileRaw(n), DeserializationOption::Filter(filter)) && p["name"].is<const char*>())
        names[n] = p["name"];
    }
    sendJson(r, d);
  });

  server.on("/api/profile", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    String name = r->hasParam("name") ? r->getParam("name")->value() : "";
    String raw = readProfileRaw(name);
    if (raw.isEmpty()) return sendMsg(r, false, T("Profil nicht gefunden", "Profile not found"), 404);
    r->send(200, "application/json", raw);
  });

  postJson("/api/profile", [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    String name = r->hasParam("name") ? r->getParam("name")->value() : "";
    String err;
    if (!saveProfileRaw(name, body(r), err)) return sendMsg(r, false, err);
    bool active;
    {
      std::lock_guard<std::mutex> lk(Poller::mutex());
      active = (name == cfg.profile);
    }
    if (active) Poller::requestProfileReload();
    logf(T("Profil '%s' gespeichert", "Profile '%s' saved"), name.c_str());
    sendMsg(r, true, active ? T("Gespeichert und aktiviert", "Saved and activated") : T("Gespeichert", "Saved"));
  });

  server.on("/api/profile-delete", HTTP_POST, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    String name = r->hasParam("name") ? r->getParam("name")->value() : "";
    bool active;
    {
      std::lock_guard<std::mutex> lk(Poller::mutex());
      active = (name == cfg.profile);
    }
    if (active) return sendMsg(r, false, T("Aktives Profil kann nicht gelöscht werden", "The active profile cannot be deleted"));
    if (!validProfileName(name) || !LittleFS.remove("/profiles/" + name + ".json"))
      return sendMsg(r, false, T("Löschen fehlgeschlagen", "Delete failed"));
    sendMsg(r, true, T("Gelöscht", "Deleted"));
  });

  server.on("/api/profile-defaults", HTTP_POST, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    installDefaultProfiles(true);
    Poller::requestProfileReload();
    sendMsg(r, true, T("Werksprofile wiederhergestellt", "Factory profiles restored"));
  });

  postJson("/api/job", [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    JsonDocument d;
    if (deserializeJson(d, body(r))) return sendMsg(r, false, T("Ungültiges JSON", "Invalid JSON"));
    String t = d["type"] | "";
    Poller::JobType jt = t == "scan" ? Poller::JOB_SCAN : t == "test" ? Poller::JOB_TEST : t == "diag" ? Poller::JOB_DIAG : Poller::JOB_RAW;
    String cmd = d["cmd"] | "";
    if (jt != Poller::JOB_SCAN && jt != Poller::JOB_DIAG && cmd.isEmpty()) return sendMsg(r, false, T("Befehl fehlt", "Command missing"));
    if (Poller::isPaused()) return sendMsg(r, false, T("Im sicheren Modus / während eines Updates nicht verfügbar", "Not available in safe mode / during an update"), 409);
    if (!Poller::submitJob(jt, d["header"] | "", cmd, d["formula"] | ""))
      return sendMsg(r, false, T("Es läuft bereits ein Auftrag", "Another job is already running"), 409);
    sendMsg(r, true, T("Gestartet", "Started"));
  });

  server.on("/api/job", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    JsonDocument d;
    Poller::jobJson(d);
    sendJson(r, d);
  });

  // WLAN-Suche (asynchron): 1. Aufruf startet Scan, Folgeaufrufe liefern Ergebnis
  server.on("/api/wifiscan", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    JsonDocument d;
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED || r->hasParam("start")) {
      if (n != WIFI_SCAN_RUNNING) {
        WiFi.scanDelete();
        WiFi.scanNetworks(true, false);   // async, ohne versteckte Netze
      }
      d["state"] = "running";
    } else if (n == WIFI_SCAN_RUNNING) {
      d["state"] = "running";
    } else {
      d["state"] = "done";
      JsonArray arr = d["networks"].to<JsonArray>();
      for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.isEmpty()) continue;
        bool dup = false;   // gleiche SSID (mehrere APs/Mesh) nur einmal, stärkstes Signal
        for (JsonObject o : arr)
          if (o["ssid"] == ssid) {
            dup = true;
            if (WiFi.RSSI(i) > o["rssi"].as<int>()) o["rssi"] = WiFi.RSSI(i);
          }
        if (dup) continue;
        JsonObject o = arr.add<JsonObject>();
        o["ssid"] = ssid;
        o["rssi"] = WiFi.RSSI(i);
        o["ch"] = WiFi.channel(i);
        o["open"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
      }
      WiFi.scanDelete();
    }
    sendJson(r, d);
  });

  // Log herunterladen: ?old=1 = vorherige (rotierte) Datei, sonst aktuelles Log
  server.on("/api/log", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    bool old = r->hasParam("old");
    logFlush();
    // Datei unter Sperre komplett lesen (max. ~20 KB) statt zu streamen – sonst kann die Hauptschleife
    // die Datei während der Übertragung rotieren/löschen (Absturzursache bis 0.3.5)
    // Stückweise aus dem Flash senden (je max. 1 KB unter Sperre) – kein großer RAM-Puffer
    AsyncWebServerResponse* res;
    if (logPersist() && logFileExists(old)) {
      res = r->beginChunkedResponse("text/plain; charset=utf-8", [old](uint8_t* buf, size_t maxLen, size_t index) -> size_t {
        return logReadChunk(old, index, buf, maxLen);
      });
    } else if (old) {
      return sendMsg(r, false, T("Kein älteres Log vorhanden", "No older log available"), 404);
    } else {
      res = r->beginResponse(200, "text/plain; charset=utf-8", logText());
    }
    if (r->hasParam("dl"))
      res->addHeader("Content-Disposition", String("attachment; filename=\"obd2mqtt-") +
                                              (old ? "log-alt" : "log") + ".txt\"");
    r->send(res);
  });

  server.on("/api/log-clear", HTTP_POST, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    logClear();
    logf("%s", T("Log gelöscht", "Log cleared"));
    sendMsg(r, true, T("Log gelöscht", "Log cleared"));
  });

  // ---------- Sichern & Wiederherstellen ----------
  // GET /api/backup?secrets=1  → JSON-Datei mit Einstellungen + allen Profilen
  server.on("/api/backup", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    bool secrets = r->hasParam("secrets") && r->getParam("secrets")->value() == "1";
    JsonDocument d;
    d["type"] = "obd2mqtt-backup";
    d["format"] = 1;
    d["fw"] = FW_VERSION;
    char ts[24] = "";
    if (timeValid()) {
      time_t now = time(nullptr);
      struct tm t;
      localtime_r(&now, &t);
      strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &t);
      d["created"] = ts;
    }
    d["with_secrets"] = secrets;
    {
      JsonDocument c;
      {
        std::lock_guard<std::mutex> lk(Poller::mutex());
        configToJson(c, !secrets);
      }
      d["config"] = c;
    }
    JsonObject pr = d["profiles"].to<JsonObject>();
    for (auto& n : listProfiles()) {
      JsonDocument p;
      if (!deserializeJson(p, readProfileRaw(n))) pr[n] = p;
    }
    String out;
    serializeJsonPretty(d, out);
    AsyncWebServerResponse* res = r->beginResponse(200, "application/json", out);
    String date = ts[0] ? String(ts).substring(0, 10) : String("backup");
    res->addHeader("Content-Disposition", "attachment; filename=\"obd2mqtt-backup-" + date + ".json\"");
    r->send(res);
  });

  // POST /api/restore?config=1&profiles=1  (Body = Backup-JSON) → danach Neustart
  postJson("/api/restore", [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    bool doCfg = !r->hasParam("config") || r->getParam("config")->value() == "1";
    bool doProf = !r->hasParam("profiles") || r->getParam("profiles")->value() == "1";
    JsonDocument d;
    if (body(r).isEmpty()) return sendMsg(r, false, T("Datei leer oder zu groß (max. 64 KB)", "File empty or too large (max. 64 KB)"));
    if (auto e = deserializeJson(d, body(r))) return sendMsg(r, false, String(T("Keine gültige JSON-Datei: ", "Not a valid JSON file: ")) + e.c_str());
    if (d["type"] != "obd2mqtt-backup") return sendMsg(r, false, T("Das ist keine OBD2MQTT-Sicherung", "This is not an OBD2MQTT backup"));
    int okP = 0;
    String errs;
    if (doProf) {
      for (JsonPair kv : d["profiles"].as<JsonObject>()) {
        String js, err;
        serializeJson(kv.value(), js);
        if (saveProfileRaw(kv.key().c_str(), js, err)) okP++;
        else errs += String(kv.key().c_str()) + ": " + err + "; ";
      }
    }
    bool cfgDone = false;
    if (doCfg && d["config"].is<JsonObject>()) {
      String js;
      serializeJson(d["config"], js);
      Poller::requestConfigApply(js);
      cfgDone = true;
    }
    if (!cfgDone && okP == 0) return sendMsg(r, false, String(T("Nichts wiederhergestellt. ", "Nothing restored. ")) + errs);
    String msg = T("Wiederhergestellt: ", "Restored: ");
    if (cfgDone) msg += String(T("Einstellungen", "settings")) + (d["with_secrets"] ? "" : T(" (Passwörter unverändert)", " (passwords unchanged)"));
    if (okP) msg += String(cfgDone ? " + " : "") + okP + T(" Profil(e)", " profile(s)");
    if (errs.length()) msg += String(T(". Fehler: ", ". Errors: ")) + errs;
    msg += T(" – Neustart…", " – restarting…");
    logf(T("Backup wiederhergestellt (%s)", "Backup restored (%s)"), d["created"] | "-");
    rebootAt = millis() + 2500;   // WLAN/MQTT-Einstellungen wirken erst nach Neustart
    sendMsg(r, true, msg);
  });

  server.on("/api/pollnow", HTTP_POST, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    Poller::requestPollNow();
    sendMsg(r, true, T("Abfrage angestoßen", "Poll triggered"));
  });

  server.on("/api/diag", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    AsyncWebServerResponse* res = r->beginChunkedResponse("text/plain; charset=utf-8", [](uint8_t* buf, size_t maxLen, size_t index) -> size_t {
      return Poller::diagChunk(index, buf, maxLen);
    });
    if (r->hasParam("dl")) res->addHeader("Content-Disposition", "attachment; filename=\"obd2mqtt-diagnose.txt\"");
    r->send(res);
  });

  server.on("/api/debug", HTTP_POST, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    uint32_t m = r->hasParam("min") ? r->getParam("min")->value().toInt() : 0;
    if (m > 720) m = 720;
    Poller::setDebug(m);
    sendMsg(r, true, m ? T("Diagnose-Log eingeschaltet", "Diagnostic log switched on") : T("Diagnose-Log ausgeschaltet", "Diagnostic log switched off"));
  });

  server.on("/api/polling", HTTP_POST, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    bool on = r->hasParam("on") && r->getParam("on")->value() == "1";
    Poller::requestEnabled(on);
    sendMsg(r, true, on ? T("Abfrage eingeschaltet", "Polling switched on") : T("Abfrage ausgeschaltet", "Polling switched off"));
  });

  server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    Watchdog::clearCrashCounter();   // manueller Neustart → sicherer Modus endet
    rebootAt = millis() + 1000;
    sendMsg(r, true, T("Neustart…", "Restarting…"));
  });

  // OTA-Firmware-Update (firmware.bin)
  // OTA-Firmware-Update (obd2mqtt-vX.Y.Z-esp32-ota.bin)
  server.on("/api/update", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!auth(r)) return;
    JsonDocument d;
    static const char* ST[] = {"idle", "running", "done", "error"};
    d["state"] = ST[otaState];
    d["written"] = (uint32_t)otaWritten;
    d["total"] = (uint32_t)otaTotal;
    d["error"] = otaError;
    sendJson(r, d);
  });

  server.on("/api/update", HTTP_POST,
    [](AsyncWebServerRequest* r) {
      if (!auth(r)) return;
      bool ok = otaState == 2;
      if (!ok && otaState != 3) { otaState = 3; if (otaError.isEmpty()) otaError = T("Keine Daten empfangen", "No data received"); }
      sendMsg(r, ok, ok ? String(T("Update erfolgreich – Neustart…", "Update successful – restarting…")) : String(T("Update fehlgeschlagen: ", "Update failed: ")) + otaError);
      if (ok) rebootAt = millis() + 1500;
      else Poller::setPaused(false);
    },
    [](AsyncWebServerRequest* r, const String& fn, size_t index, uint8_t* data, size_t len, bool final) {
      if (!cfg.webPass.isEmpty() && !r->authenticate("admin", cfg.webPass.c_str())) return;
      if (index == 0) {
        otaError = "";
        otaWritten = 0;
        otaTotal = r->contentLength();
        otaState = 1;
        Poller::setPaused(true);   // keine BLE-Abfragen während des Flashens
        logf(T("OTA-Update gestartet: %s (%u Bytes)", "OTA update started: %s (%u bytes)"), fn.c_str(), (unsigned)otaTotal);
        if (Update.isRunning()) Update.abort();       // Reste eines abgebrochenen Versuchs
        if (len && data[0] != 0xE9) {
          otaError = fn.indexOf("factory") >= 0
                       ? T("Das ist die factory.bin – für OTA bitte die *-ota.bin verwenden", "This is the factory.bin – please use the *-ota.bin for OTA")
                       : T("Keine gültige ESP32-Firmware (falsches Magic-Byte)", "Not a valid ESP32 firmware (wrong magic byte)");
          otaState = 3;
        } else if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
          otaError = Update.errorString();
          otaState = 3;
        }
      }
      if (otaState != 1) return;
      if (len && Update.write(data, len) != len) {
        otaError = Update.errorString();
        otaState = 3;
        Update.abort();
        logf(T("OTA-Fehler: %s", "OTA error: %s"), otaError.c_str());
        return;
      }
      otaWritten = index + len;
      if (final) {
        if (Update.end(true)) {
          otaState = 2;
          logf(T("OTA-Update fertig (%u Bytes) – Neustart", "OTA update finished (%u bytes) – restarting"), (unsigned)otaWritten);
        } else {
          otaError = Update.errorString();
          otaState = 3;
          logf(T("OTA-Fehler: %s", "OTA error: %s"), otaError.c_str());
        }
      }
    });

  server.onNotFound([](AsyncWebServerRequest* r) {
    // Captive-Portal: unbekannte Pfade auf die Startseite umleiten
    r->redirect("/");
  });

  server.begin();
}

void loop() {
  if (rebootAt && (int32_t)(millis() - rebootAt) >= 0) {
    logf("%s", T("Neustart über Weboberfläche", "Restart via web interface"));
    logFlush();
    delay(100);
    ESP.restart();
  }
}

}  // namespace Web
