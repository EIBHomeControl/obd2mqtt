#pragma once
#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="de"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>OBD2MQTT</title>
<style>
:root{--bg:#f4f5f7;--card:#fff;--fg:#1d2330;--mut:#6b7385;--bd:#dfe3ea;--acc:#1f6feb;--ok:#1a7f37;--err:#cf222e;--warn:#9a6700;--in:#fff}
@media(prefers-color-scheme:dark){:root{--bg:#0f1218;--card:#181c24;--fg:#e6e9ef;--mut:#8b93a5;--bd:#2a303c;--acc:#4c8dff;--ok:#3fb950;--err:#f85149;--warn:#d29922;--in:#11151c}}
*{box-sizing:border-box}body{margin:0;font:15px/1.45 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;background:var(--bg);color:var(--fg)}
header{display:flex;align-items:center;gap:12px;padding:12px 16px;background:var(--card);border-bottom:1px solid var(--bd);position:sticky;top:0;z-index:5;flex-wrap:wrap}
header h1{font-size:17px;margin:0;font-weight:650}header .sub{color:var(--mut);font-size:13px}
nav{display:flex;gap:4px;margin-left:auto;flex-wrap:wrap}
nav button{background:none;border:0;color:var(--mut);padding:7px 11px;border-radius:7px;cursor:pointer;font:inherit}
nav button.on{background:var(--bg);color:var(--fg);font-weight:600}
main{max-width:1100px;margin:0 auto;padding:16px}
.card{background:var(--card);border:1px solid var(--bd);border-radius:12px;padding:16px;margin-bottom:16px}
.card h2{font-size:15px;margin:0 0 12px}
.grid{display:grid;gap:12px;grid-template-columns:repeat(auto-fit,minmax(170px,1fr))}
.val{border:1px solid var(--bd);border-radius:10px;padding:12px}
.val .l{color:var(--mut);font-size:13px}.val .v{font-size:30px;font-weight:650;font-variant-numeric:tabular-nums}
.val .v small{font-size:15px;color:var(--mut);font-weight:500;margin-left:3px}.val .m{font-size:12px;color:var(--mut)}
.val .e{font-size:12px;color:var(--err);margin-top:4px;word-break:break-word}
.pill{display:inline-flex;align-items:center;gap:6px;font-size:13px;padding:3px 9px;border-radius:99px;border:1px solid var(--bd)}
.dot{width:8px;height:8px;border-radius:50%;background:var(--mut)}.dot.ok{background:var(--ok)}.dot.err{background:var(--err)}
.row{display:flex;gap:8px;flex-wrap:wrap;align-items:center}
label{display:block;font-size:13px;color:var(--mut);margin:0 0 4px}
.f{display:grid;gap:12px;grid-template-columns:repeat(auto-fit,minmax(220px,1fr))}
input,select,textarea{width:100%;font:inherit;padding:7px 9px;border:1px solid var(--bd);border-radius:7px;background:var(--in);color:var(--fg)}
textarea{font-family:ui-monospace,Menlo,Consolas,monospace;font-size:13px}
input[type=checkbox]{width:auto}
button.b,a.b{display:inline-block;font:inherit;font-size:15px;line-height:1.45;padding:7px 13px;border-radius:7px;border:1px solid var(--bd);background:var(--card);color:var(--fg);cursor:pointer}
button.p,a.p{background:var(--acc);border-color:var(--acc);color:#fff}button.d,a.d{color:var(--err)}
button:disabled{opacity:.5;cursor:default}
.hint{color:var(--mut);font-size:13px;margin:6px 0 0}
pre,.mono{font-family:ui-monospace,Menlo,Consolas,monospace;font-size:12.5px}
pre{background:var(--in);border:1px solid var(--bd);border-radius:8px;padding:10px;overflow:auto;max-height:320px;margin:0;white-space:pre-wrap;word-break:break-all}
.tw{overflow-x:auto}table{border-collapse:collapse;width:100%;font-size:13px}
th,td{padding:5px 4px;border-bottom:1px solid var(--bd);text-align:left;vertical-align:middle}th{color:var(--mut);font-weight:500;white-space:nowrap}
td input,td select{padding:4px 6px;font-size:13px;min-width:44px}
td input[type=checkbox]{min-width:0;width:18px;height:18px}td select{min-width:100px}
.bytes{display:flex;flex-wrap:wrap;gap:3px}.bytes span{border:1px solid var(--bd);border-radius:5px;padding:2px 4px;text-align:center;min-width:44px}
.bytes b{display:block;font-size:10px;color:var(--mut);font-weight:500}
.toast{position:fixed;bottom:16px;left:50%;transform:translateX(-50%);background:var(--fg);color:var(--bg);padding:9px 16px;border-radius:9px;opacity:0;transition:.25s;pointer-events:none;z-index:9}
.toast.show{opacity:1}.hide{display:none!important}
.scanl div{display:flex;justify-content:space-between;gap:8px;padding:7px 4px;border-bottom:1px solid var(--bd);cursor:pointer}
.scanl div:hover{background:var(--bg)}
</style></head><body>
<header><div><h1>OBD2MQTT</h1><div class="sub" id="hsub">BLE-OBD2 → MQTT Bridge</div></div>
<nav id="nav"><button data-t="status" class="on">Status</button><button data-t="profile">Profil</button><button data-t="term">Terminal</button><button data-t="cfg">Einstellungen</button><button data-t="sys">System</button><a id="helplink" href="/hilfe" target="_blank" style="color:var(--acc);text-decoration:none;padding:7px 11px">Hilfe ↗</a>
<select id="langsel" title="Sprache / Language" onchange="setLang(this.value,true)" style="width:auto;padding:5px 6px;font-size:13px"><option value="de">🇩🇪 DE</option><option value="en">🇬🇧 EN</option></select></nav></header>
<main>

<section id="t-status">
 <div class="card hide" id="safebox" style="border-color:var(--err)"><h2 style="color:var(--err)">Sicherer Modus</h2>
  <p style="margin:0">Die Firmware ist mehrmals hintereinander abgestürzt. Bluetooth und Abfragen sind deshalb deaktiviert, die Weboberfläche läuft. Prüfe Profil und Einstellungen und starte dann normal neu.</p>
  <div class="row" style="margin-top:10px"><button class="b p" onclick="api('/api/reboot',{method:'POST'}).then(r=>toast(r.msg))">Normal neu starten</button></div></div>
 <div class="card"><div class="row" style="justify-content:space-between"><h2 style="margin:0">Werte</h2><button class="b" onclick="pollNow()">Jetzt abfragen</button></div>
  <div class="grid" id="vals" style="margin-top:12px"></div></div>
 <div class="card"><h2>Verbindung</h2><div class="row" id="conn"></div><p class="hint" id="lerr"></p></div>
 <div class="card"><div class="row" style="justify-content:space-between"><h2 style="margin:0">Log</h2>
  <div class="row"><button class="b" onclick="copyLog()">Kopieren</button><a class="b" style="text-decoration:none" href="/api/log?dl=1">Herunterladen</a><a class="b" style="text-decoration:none" href="/api/log?old=1&amp;dl=1">Älteres Log</a><button class="b d" onclick="if(confirm('Log löschen?'))api('/api/log-clear',{method:'POST'}).then(r=>{toast(r.msg);status()})">Löschen</button></div></div>
  <pre id="log" style="max-height:300px;margin-top:12px"></pre>
  <p class="hint">Angezeigt werden die letzten 60 Zeilen. „Kopieren“ und „Herunterladen“ liefern das komplette Log – bei aktiver Speicherung auch über Neustarts hinweg.</p></div>
</section>

<section id="t-profile" class="hide">
 <div class="card"><div class="row">
  <div style="flex:1;min-width:160px"><label>Profil</label><select id="psel" onchange="loadProfile(this.value)"></select></div>
  <div style="align-self:end" class="row"><button class="b p" onclick="activate()">Aktivieren</button><button class="b" onclick="saveAs()">Kopie speichern…</button><button class="b d" onclick="delProfile()">Löschen</button></div></div>
  <p class="hint" id="pact"></p></div>
 <div class="card"><h2>Allgemein</h2><div class="f">
  <div><label>Name</label><input id="p_name"></div>
  <div><label>Modell (für Home Assistant)</label><input id="p_model"></div>
  <div><label>Init-Befehle (einer pro Zeile)</label><textarea id="p_init" rows="4"></textarea></div></div></div>
 <div class="card"><div class="row" style="justify-content:space-between"><h2 style="margin:0">PIDs</h2><button class="b" onclick="addPid()">+ PID</button></div>
  <p class="hint" data-en="Formula: &lt;span class=&quot;mono&quot;&gt;B0..Bn&lt;/span&gt; are the response bytes &lt;b&gt;including&lt;/b&gt; service bytes (for &lt;span class=&quot;mono&quot;&gt;220105&lt;/span&gt;: B0=62, B1=01, B2=05). Functions: &lt;span class=&quot;mono&quot;&gt;u16(hi,lo) s16(hi,lo) s8(x) bit(x,n)&lt;/span&gt;. Use &lt;b&gt;Test&lt;/b&gt; to see all bytes with their index.">Formel: <span class="mono">B0..Bn</span> sind die Antwort-Bytes <b>inkl.</b> Service-Bytes (bei <span class="mono">220105</span>: B0=62, B1=01, B2=05). Funktionen: <span class="mono">u16(hi,lo) s16(hi,lo) s8(x) bit(x,n)</span>. Mit <b>Test</b> siehst du alle Bytes mit Index.</p>
  <div class="tw"><table><thead><tr><th><label style="display:flex;gap:4px;align-items:center;margin:0;color:inherit"><input type="checkbox" id="allpids" onchange="toggleAll(this.checked)" title="Alle an/aus"> <span>Aktiv</span></label></th><th>ID</th><th>Name</th><th>Header</th><th>Befehl</th><th>Formel</th><th>Einheit</th><th>Klasse</th><th>Intervall s</th><th>Min</th><th>Max</th><th>Nk</th><th></th></tr></thead><tbody id="pids"></tbody></table></div>
  <div class="row" style="margin-top:12px"><button class="b p" onclick="saveProfile()">Profil speichern</button><button class="b" onclick="toggleRaw()">JSON bearbeiten</button></div>
  <div id="rawbox" class="hide" style="margin-top:12px"><textarea id="raw" rows="16"></textarea><div class="row" style="margin-top:8px"><button class="b" onclick="rawApply()">JSON übernehmen</button></div></div>
 </div>
 <div class="card hide" id="testcard"><h2>Testergebnis <span id="testid" class="mono"></span></h2><div id="testout"></div></div>
</section>

<section id="t-term" class="hide">
 <div class="card"><h2>ELM327-Terminal</h2>
  <div class="row"><input id="tcmd" class="mono" style="flex:1;min-width:180px" placeholder="z.B. ATRV, ATSH7E4, 220105" onkeydown="if(event.key==='Enter')termSend()"><button class="b p" onclick="termSend()">Senden</button></div>
  <div class="row" style="margin-top:8px"><span class="hint" style="margin:0">Schnell:</span>
   <button class="b" onclick="q('ATI')">ATI</button><button class="b" onclick="q('ATRV')">ATRV</button><button class="b" onclick="q('ATDP')">ATDP</button><button class="b" onclick="q('0100')">0100</button><button class="b" onclick="q('ATSH7E4')">ATSH7E4</button><button class="b" onclick="q('220105')">220105</button></div>
  <pre id="tout" style="margin-top:12px;min-height:200px"></pre>
  <p class="hint">Befehle laufen zwischen den automatischen Abfragen. Nach ATSH-Befehlen setzt die Abfrage den Header selbst wieder.</p></div>
</section>

<section id="t-cfg" class="hide">
 <div class="card"><h2>WLAN & Zugang</h2><div class="f">
  <div><label>SSID</label><div class="row" style="flex-wrap:nowrap"><input id="wifi_ssid"><button class="b" id="wscanbtn" onclick="wscan()" style="white-space:nowrap">Suchen</button></div></div>
  <div><label>Passwort</label><input id="wifi_pass" type="password" autocomplete="new-password"></div>
  <div><label>Hostname</label><input id="hostname"></div>
  <div><label>Web-Passwort (Benutzer admin, leer = keins)</label><input id="web_pass" type="password" autocomplete="new-password"></div>
  <div><label>IP-Adresse</label><select id="ip_mode" onchange="ipModeChange()"><option value="dhcp">automatisch (DHCP)</option><option value="static">feste IP-Adresse</option></select></div></div>
  <div class="scanl" id="wscanl" style="margin-top:8px"></div>
  <div id="ipbox" class="hide" style="margin-top:10px"><div class="f">
   <div><label>Feste IP-Adresse</label><input id="ip_addr" class="mono" placeholder="192.168.2.50"></div>
   <div><label>Subnetzmaske</label><input id="ip_mask" class="mono" placeholder="255.255.255.0"></div>
   <div><label>Gateway (Router)</label><input id="ip_gw" class="mono" placeholder="192.168.2.1"></div>
   <div><label>DNS-Server (leer = Gateway)</label><input id="ip_dns1" class="mono"></div>
   <div><label>2. DNS-Server (optional)</label><input id="ip_dns2" class="mono"></div></div>
   <div class="row" style="margin-top:8px"><button class="b" onclick="ipTakeCurrent()">Aktuelle Werte übernehmen</button></div>
   <p class="hint">Die Adresse muss außerhalb des DHCP-Bereichs deines Routers liegen. Funktioniert die feste IP nach dem Neustart nicht (keine WLAN-Verbindung oder MQTT-Broker nicht erreichbar), holt sich die Bridge automatisch eine Adresse per DHCP und meldet das im Log und unter System.</p>
   <p class="hint" id="ipfb" style="color:var(--err)"></p></div></div>
 <div class="card"><h2>BLE-Dongle</h2><div class="f">
  <div><label>MAC-Adresse</label><input id="ble_mac" class="mono" placeholder="AA:BB:CC:DD:EE:FF"></div>
  <div><label>Adresstyp</label><select id="ble_addr_type"><option value="auto">automatisch</option><option value="public">public</option><option value="random">random</option></select></div>
  <div><label>Service-UUID (leer = auto)</label><input id="ble_service" class="mono"></div>
  <div><label>Notify-UUID (leer = auto)</label><input id="ble_notify" class="mono"></div>
  <div><label>Write-UUID (leer = auto)</label><input id="ble_write" class="mono"></div><input type="hidden" id="ble_name"></div>
  <div class="row" style="margin-top:12px"><button class="b" id="scanbtn" onclick="scan()">Nach Dongles suchen</button><span class="hint" id="uuids" style="margin:0"></span></div>
  <div class="scanl" id="scanl" style="margin-top:8px"></div></div>
 <div class="card"><h2>Abfrage</h2><div class="f">
  <div><label>12V-Mindestspannung (0 = aus)</label><input id="min_voltage" type="number" step="0.1"></div>
  <div><label>Neuer Verbindungsversuch nach (s)</label><input id="retry_sec" type="number"></div>
  <div><label>Pause wenn Auto nicht antwortet (s, 0 = aus)</label><input id="backoff_sec" type="number"></div>
  <div><label>NTP-Server (Uhrzeit)</label><input id="ntp_server" placeholder="pool.ntp.org oder fritz.box"></div>
  <div><label>Zeitzone</label><select id="tzsel" onchange="tzChange()"><option value="CET-1CEST,M3.5.0,M10.5.0/3">Mitteleuropa – Deutschland, Österreich, Schweiz, Italien, Frankreich …</option><option value="GMT0BST,M3.5.0/1,M10.5.0">Großbritannien, Irland</option><option value="WET0WEST,M3.5.0/1,M10.5.0">Portugal, Kanaren</option><option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Osteuropa – Finnland, Griechenland, Rumänien, Baltikum …</option><option value="&lt;+03&gt;-3">Türkei</option><option value="MSK-3">Moskau</option><option value="UTC0">UTC (ohne Sommerzeit)</option><option value="EST5EDT,M3.2.0,M11.1.0">USA/Kanada – Eastern</option><option value="CST6CDT,M3.2.0,M11.1.0">USA/Kanada – Central</option><option value="MST7MDT,M3.2.0,M11.1.0">USA/Kanada – Mountain</option><option value="PST8PDT,M3.2.0,M11.1.0">USA/Kanada – Pacific</option><option value="CST-8">China</option><option value="JST-9">Japan</option><option value="AEST-10AEDT,M10.1.0,M4.1.0/3">Australien – Sydney, Melbourne</option><option value="custom">Andere (POSIX-Angabe) …</option></select><input id="tz" class="mono hide" style="margin-top:6px" placeholder="z. B. CET-1CEST,M3.5.0,M10.5.0/3"><p class="hint" id="devtime"></p></div>
  <div><label>Log dauerhaft speichern (Flash)</label><select id="log_persist"><option value="true">ja – übersteht Neustarts</option><option value="false">nein – nur im RAM</option></select></div>
  <div><label>Befehls-Timeout (ms)</label><input id="cmd_timeout_ms" type="number"></div>
  <div><label>BLE-Verbindung dauerhaft halten</label><select id="keep_connected"><option value="false">nein – Dongle darf schlafen</option><option value="true">ja</option></select></div></div></div>
 <div class="card"><h2>MQTT / Home Assistant</h2><div class="f">
  <div><label>Broker</label><input id="mqtt_host" placeholder="192.168.1.10"></div>
  <div><label>Port</label><input id="mqtt_port" type="number"></div>
  <div><label>Benutzer</label><input id="mqtt_user" autocomplete="off"></div>
  <div><label>Passwort</label><input id="mqtt_pass" type="password" autocomplete="new-password"></div>
  <div><label>Basis-Topic</label><input id="mqtt_base"></div>
  <div><label>Discovery-Prefix</label><input id="ha_prefix"></div>
  <div><label>Gerätename in HA</label><input id="device_name"></div></div></div>
 <div class="row"><button class="b p" onclick="saveCfg(false)">Speichern</button><button class="b" onclick="saveCfg(true)">Speichern & Neustart</button>
  <span class="hint" style="margin:0">WLAN-, MQTT- und Passwort-Änderungen werden nach dem Neustart wirksam.</span></div>
</section>

<section id="t-sys" class="hide">
 <div class="card"><h2>System</h2><div id="sysinfo" class="mono"></div>
  <div class="row" style="margin-top:12px"><button class="b" onclick="api('/api/reboot',{method:'POST'}).then(r=>toast(r.msg))">Neustart</button>
  <button class="b d" onclick="if(confirm(t('Werksprofile (ioniq5, g9, g6, p7plus) überschreiben?')))api('/api/profile-defaults',{method:'POST'}).then(r=>{toast(r.msg);loadProfiles()})">Werksprofile wiederherstellen</button></div></div>
 <div class="card"><h2>Sichern &amp; Wiederherstellen</h2>
  <p class="hint" style="margin:0 0 10px">Speichert alle Einstellungen und Fahrzeugprofile in einer Datei – z. B. vor einem Neu-Flashen oder um ein zweites Gerät gleich einzurichten.</p>
  <div class="row"><a class="b p" id="bkbtn" style="text-decoration:none" href="/api/backup?secrets=0">Sicherung herunterladen</a>
   <label style="margin:0;display:flex;gap:6px;align-items:center;color:var(--fg)"><input type="checkbox" id="bksec" onchange="$('bkbtn').href='/api/backup?secrets='+(this.checked?1:0)"> inkl. Passwörter (WLAN, MQTT, Web)</label></div>
  <p class="hint">Ohne Passwörter ist die Datei unbedenklich weiterzugeben; beim Wiederherstellen bleiben die aktuell gespeicherten Passwörter dann erhalten.</p>
  <div style="border-top:1px solid var(--bd);margin-top:12px;padding-top:12px">
   <input type="file" id="rsfile" accept=".json,application/json">
   <div class="row" style="margin-top:8px"><label style="margin:0;display:flex;gap:6px;align-items:center;color:var(--fg)"><input type="checkbox" id="rscfg" checked> Einstellungen</label>
    <label style="margin:0;display:flex;gap:6px;align-items:center;color:var(--fg)"><input type="checkbox" id="rsprof" checked> Profile</label>
    <button class="b" id="rsbtn" onclick="restore()">Wiederherstellen</button><span id="rsst" class="hint" style="margin:0"></span></div></div></div>
 <div class="card"><h2>Changelog</h2><div id="changelog" style="max-height:360px;overflow:auto"></div></div>
 <div class="card"><h2>Firmware-Update</h2><p class="hint" id="fwcur" style="margin:0 0 10px"></p><input type="file" id="fw" accept=".bin"><div class="row" style="margin-top:8px"><button class="b p" id="otabtn" onclick="ota()">Hochladen</button><span id="otast" class="hint" style="margin:0"></span></div>
  <progress id="otabar" max="100" value="0" class="hide" style="width:100%;height:14px;margin-top:10px"></progress>
  <p class="hint">Während des Updates pausieren die BLE-Abfragen. Danach startet der ESP32 neu – die Seite wartet und zeigt die neue Version.</p></div>
</section>
</main><div class="toast" id="toast"></div>
<script>
const EN={"BLE-OBD2 → MQTT Bridge": "BLE-OBD2 → MQTT bridge", "Profil": "Profile", "Einstellungen": "Settings", "Hilfe ↗": "Help ↗", "Sicherer Modus": "Safe mode", "Die Firmware ist mehrmals hintereinander abgestürzt. Bluetooth und Abfragen sind deshalb deaktiviert, die Weboberfläche läuft. Prüfe Profil und Einstellungen und starte dann normal neu.": "The firmware crashed several times in a row. Bluetooth and polling are therefore disabled, the web interface keeps running. Check profile and settings, then restart normally.", "Normal neu starten": "Restart normally", "Werte": "Values", "Jetzt abfragen": "Poll now", "Verbindung": "Connection", "Kopieren": "Copy", "Herunterladen": "Download", "Älteres Log": "Older log", "Löschen": "Delete", "Angezeigt werden die letzten 60 Zeilen. „Kopieren“ und „Herunterladen“ liefern das komplette Log – bei aktiver Speicherung auch über Neustarts hinweg.": "Showing the last 60 lines. “Copy” and “Download” return the complete log – across restarts if log storage is enabled.", "Aktivieren": "Activate", "Kopie speichern…": "Save copy…", "Allgemein": "General", "Modell (für Home Assistant)": "Model (for Home Assistant)", "Init-Befehle (einer pro Zeile)": "Init commands (one per line)", "Aktiv": "Active", "Befehl": "Command", "Formel": "Formula", "Einheit": "Unit", "Klasse": "Class", "Intervall s": "Interval s", "Nk": "Dec", "Profil speichern": "Save profile", "JSON bearbeiten": "Edit JSON", "JSON übernehmen": "Apply JSON", "Testergebnis": "Test result", "ELM327-Terminal": "ELM327 terminal", "Senden": "Send", "Schnell:": "Quick:", "Befehle laufen zwischen den automatischen Abfragen. Nach ATSH-Befehlen setzt die Abfrage den Header selbst wieder.": "Commands run between the automatic polls. After ATSH commands the poller sets the header again by itself.", "WLAN & Zugang": "WiFi & access", "Suchen": "Scan", "Passwort": "Password", "Web-Passwort (Benutzer admin, leer = keins)": "Web password (user admin, empty = none)", "MAC-Adresse": "MAC address", "Adresstyp": "Address type", "automatisch": "automatic", "Service-UUID (leer = auto)": "Service UUID (empty = auto)", "Notify-UUID (leer = auto)": "Notify UUID (empty = auto)", "Write-UUID (leer = auto)": "Write UUID (empty = auto)", "Nach Dongles suchen": "Scan for dongles", "Abfrage": "Polling", "12V-Mindestspannung (0 = aus)": "12V minimum voltage (0 = off)", "Neuer Verbindungsversuch nach (s)": "Reconnect attempt after (s)", "Pause wenn Auto nicht antwortet (s, 0 = aus)": "Pause when car does not respond (s, 0 = off)", "NTP-Server (Uhrzeit)": "NTP server (time)", "Zeitzone (POSIX)": "Time zone (POSIX)", "Log dauerhaft speichern (Flash)": "Store log permanently (flash)", "ja – übersteht Neustarts": "yes – survives restarts", "nein – nur im RAM": "no – RAM only", "Befehls-Timeout (ms)": "Command timeout (ms)", "BLE-Verbindung dauerhaft halten": "Keep BLE connected permanently", "nein – Dongle darf schlafen": "no – dongle may sleep", "ja": "yes", "Benutzer": "User", "Basis-Topic": "Base topic", "Discovery-Prefix": "Discovery prefix", "Gerätename in HA": "Device name in HA", "Speichern": "Save", "Speichern & Neustart": "Save & restart", "WLAN-, MQTT- und Passwort-Änderungen werden nach dem Neustart wirksam.": "WiFi, MQTT and password changes take effect after the restart.", "Neustart": "Restart", "Werksprofile wiederherstellen": "Restore factory profiles", "Sichern & Wiederherstellen": "Backup & restore", "Speichert alle Einstellungen und Fahrzeugprofile in einer Datei – z. B. vor einem Neu-Flashen oder um ein zweites Gerät gleich einzurichten.": "Saves all settings and vehicle profiles to a file – e.g. before re-flashing or to set up a second device.", "Sicherung herunterladen": "Download backup", "inkl. Passwörter (WLAN, MQTT, Web)": "incl. passwords (WiFi, MQTT, web)", "Ohne Passwörter ist die Datei unbedenklich weiterzugeben; beim Wiederherstellen bleiben die aktuell gespeicherten Passwörter dann erhalten.": "Without passwords the file is safe to share; when restoring, the currently stored passwords are kept.", "Profile": "Profiles", "Wiederherstellen": "Restore", "Hochladen": "Upload", "Während des Updates pausieren die BLE-Abfragen. Danach startet der ESP32 neu – die Seite wartet und zeigt die neue Version.": "BLE polling pauses during the update. Afterwards the ESP32 restarts – the page waits and shows the new version.", "z.B. ATRV, ATSH7E4, 220105": "e.g. ATRV, ATSH7E4, 220105", "pool.ntp.org oder fritz.box": "pool.ntp.org or fritz.box", "nie": "never", "aktualisiert": "updated", "nächste": "next", "12V Bordnetz": "12V battery", "nicht konfiguriert": "not configured", "Versuch in": "retry in", "Letzter Fehler: ": "Last error: ", "Erkannt: ": "Detected: ", "Installiert: v": "Installed: v", "Letzter Neustart": "Last restart", "Laufzeit": "Uptime", "Freier Heap": "Free heap", "Minimum": "minimum", "größter Block": "largest block", "Dateisystem": "File system", "Abfragezyklen": "Poll cycles", "Log in die Zwischenablage kopiert": "Log copied to clipboard", "Zeilen": "lines", "Kopieren nicht möglich – bitte „Herunterladen“ nutzen": "Copy not possible – please use “Download”", "Bitte Sicherungsdatei wählen": "Please choose a backup file", "Keine gültige JSON-Datei": "Not a valid JSON file", "Das ist keine OBD2MQTT-Sicherung": "This is not an OBD2MQTT backup", "Bitte Einstellungen und/oder Profile wählen": "Please select settings and/or profiles", "Sicherung vom": "Restore backup from", "wiederherstellen?": "?", " inkl. Passwörter": " incl. passwords", " (Passwörter bleiben wie sie sind)": " (passwords stay as they are)", "Profil(e) – gleichnamige werden überschrieben": "profile(s) – profiles with the same name are overwritten", "Danach startet das Gerät neu.": "The device restarts afterwards.", "Übertrage…": "Uploading…", "Fertig – Gerät neu gestartet.": "Done – device restarted.", "Sicherung wiederhergestellt": "Backup restored", "Warte auf Neustart…": "Waiting for restart…", "Gerät nicht erreichbar – falls sich das WLAN geändert hat, neue Adresse aufrufen.": "Device not reachable – if the WiFi changed, open the new address.", "Zeitüberschreitung": "Timeout", "Fehler: ": "Error: ", "Suche läuft (6 s)…": "Scanning (6 s)…", "(ohne Namen)": "(no name)", "Keine Geräte gefunden.": "No devices found.", "Suche…": "Scanning…", "Kanal": "Channel", "Keine Netze gefunden.": "No networks found.", "Passwort eingeben, dann „Speichern & Neustart“": "Enter password, then “Save & restart”", "Übernommen – Speichern nicht vergessen": "Applied – don’t forget to save", "Dieses Profil ist aktiv.": "This profile is active.", "Aktiv ist: ": "Active profile: ", "Name der Kopie (a-z, 0-9, _ -):": "Name of the copy (a-z, 0-9, _ -):", " aktiviert": " activated", " löschen?": " – delete?", "Übernommen – jetzt speichern": "Applied – now save", "JSON-Fehler: ": "JSON error: ", "Abfrage läuft…": "Polling…", "Rohantwort": "Raw response", "Bitte eine Firmware-Datei wählen": "Please choose a firmware file", " ist die factory.bin (nur für USB).\nFür das Update die *-ota.bin verwenden. Trotzdem hochladen?": " is the factory.bin (USB only).\nUse the *-ota.bin for updates. Upload anyway?", "Starte…": "Starting…", "Schreibe in den Flash": "Writing to flash", "Sende Datei…": "Sending file…", "Fehler": "Error", "Fertig – Neustart…": "Done – restarting…", "Update abgeschlossen": "Update finished", "Neue Version läuft: v": "New version running: v", "ESP32 nicht erreichbar – Seite bitte neu laden": "ESP32 not reachable – please reload the page", "Verbindung abgebrochen": "Connection lost", "Mitteleuropa – Deutschland, Österreich, Schweiz, Italien, Frankreich …": "Central Europe – Germany, Austria, Switzerland, Italy, France …", "Großbritannien, Irland": "United Kingdom, Ireland", "Portugal, Kanaren": "Portugal, Canary Islands", "Osteuropa – Finnland, Griechenland, Rumänien, Baltikum …": "Eastern Europe – Finland, Greece, Romania, Baltics …", "Türkei": "Turkey", "Moskau": "Moscow", "UTC (ohne Sommerzeit)": "UTC (no daylight saving)", "USA/Kanada – Eastern": "USA/Canada – Eastern", "USA/Kanada – Central": "USA/Canada – Central", "USA/Kanada – Mountain": "USA/Canada – Mountain", "USA/Kanada – Pacific": "USA/Canada – Pacific", "China": "China", "Japan": "Japan", "Australien – Sydney, Melbourne": "Australia – Sydney, Melbourne", "Zeitzone": "Time zone", "Andere (POSIX-Angabe) …": "Other (POSIX string) …", "z. B. CET-1CEST,M3.5.0,M10.5.0/3": "e.g. CET-1CEST,M3.5.0,M10.5.0/3", "Uhrzeit am Gerät": "Time on the device", "noch nicht synchronisiert": "not synchronized yet", "Bluetooth-Name": "Bluetooth name", "Name der Kopie:": "Name of the copy:", "Kopie": "copy", "Alle Werte aktiviert – Profil speichern nicht vergessen": "All values enabled – don’t forget to save the profile", "Alle Werte deaktiviert – Profil speichern nicht vergessen": "All values disabled – don’t forget to save the profile", "Alle an/aus": "All on/off", "Werksprofile (ioniq5, g9, g6, p7plus) überschreiben?": "Overwrite factory profiles (ioniq5, g9, g6, p7plus)?", "neu": "new", "Neu": "New", "Sprache gespeichert": "Language saved", "IP-Adresse": "IP address", "automatisch (DHCP)": "automatic (DHCP)", "feste IP-Adresse": "static IP address", "Feste IP-Adresse": "Static IP address", "Subnetzmaske": "Subnet mask", "Gateway (Router)": "Gateway (router)", "DNS-Server (leer = Gateway)": "DNS server (empty = gateway)", "2. DNS-Server (optional)": "2nd DNS server (optional)", "Aktuelle Werte übernehmen": "Use current values", "Die Adresse muss außerhalb des DHCP-Bereichs deines Routers liegen. Funktioniert die feste IP nach dem Neustart nicht (keine WLAN-Verbindung oder MQTT-Broker nicht erreichbar), holt sich die Bridge automatisch eine Adresse per DHCP und meldet das im Log und unter System.": "The address must be outside your router's DHCP range. If the static IP does not work after the restart (no WiFi connection or MQTT broker not reachable), the bridge automatically gets an address via DHCP and reports this in the log and under System.", "Ungültige IP-Angabe: ": "Invalid IP: ", "fest": "static", "DHCP-Fallback – feste IP hat nicht funktioniert": "DHCP fallback – static IP did not work", "Die feste IP hat beim letzten Start nicht funktioniert – aktuell per DHCP: ": "The static IP did not work at the last start – currently via DHCP: "};
let LANG='de';try{LANG=localStorage.getItem('lang')||'de'}catch(e){}
const t=s=>LANG==='en'&&EN[s]!==undefined?EN[s]:s;
let staticNodes=null,staticAttrs=null,staticRich=null;
function applyLang(){
 if(!staticNodes){staticNodes=[];const w=document.createTreeWalker(document.body,NodeFilter.SHOW_TEXT,{acceptNode:n=>n.parentNode.closest('script,style,pre,[data-en]')?NodeFilter.FILTER_REJECT:NodeFilter.FILTER_ACCEPT});
  while(w.nextNode()){const n=w.currentNode,k=n.nodeValue.trim();if(k&&EN[k]!==undefined){n._de=n.nodeValue;n._k=k;staticNodes.push(n)}}
  staticAttrs=[...document.querySelectorAll('[placeholder],[title]')].map(e=>({e,p:e.getAttribute('placeholder'),ti:e.getAttribute('title')}));
  staticRich=[...document.querySelectorAll('[data-en]')].map(e=>({e,de:e.innerHTML}))}
 staticNodes.forEach(n=>n.nodeValue=LANG==='en'?n._de.replace(n._k,EN[n._k]):n._de);
 staticAttrs.forEach(a=>{if(a.p)a.e.setAttribute('placeholder',t(a.p));if(a.ti)a.e.setAttribute('title',t(a.ti))});
 staticRich.forEach(r=>r.e.innerHTML=LANG==='en'?r.e.dataset.en:r.de);
 document.documentElement.lang=LANG;$('langsel').value=LANG;$('helplink').href='/hilfe?lang='+LANG;
 if(typeof renderChangelog==='function')renderChangelog();
 if(prof)renderProfile();if(typeof active!=='undefined'&&profName&&$('pact'))$('pact').textContent=profName===active?t('Dieses Profil ist aktiv.'):t('Aktiv ist: ')+pname(active);
}
async function setLang(l,save){LANG=l==='en'?'en':'de';try{localStorage.setItem('lang',LANG)}catch(e){}applyLang();status();
 if(save){const r=await post('/api/config',{lang:LANG});if(r.ok)toast(t('Sprache gespeichert'))}}
const $=id=>document.getElementById(id);
const esc=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const nl=s=>String(s).replace(/\r\n?/g,'\n');
let cur='status',prof=null,profName='',active='';
function toast(m){const t=$('toast');t.textContent=m;t.classList.add('show');clearTimeout(t._h);t._h=setTimeout(()=>t.classList.remove('show'),2600)}
async function api(u,o={}){const r=await fetch(u,o);let j;try{j=await r.json()}catch(e){j={ok:r.ok,msg:r.statusText}}if(!r.ok&&j.ok===undefined)j.ok=false;return j}
const post=(u,b)=>api(u,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b)});
document.querySelectorAll('#nav button').forEach(b=>b.onclick=()=>show(b.dataset.t));
function show(t){cur=t;document.querySelectorAll('#nav button').forEach(b=>b.classList.toggle('on',b.dataset.t===t));
 document.querySelectorAll('main>section').forEach(s=>s.classList.toggle('hide',s.id!=='t-'+t));
 if(t==='profile'&&!prof)loadProfiles();if(t==='cfg')loadCfg();if(t==='sys')status()}
const age=s=>s<0?t('nie'):s<60?s+' s':s<3600?Math.round(s/60)+' min':Math.round(s/3600)+' h';
function pill(ok,txt){return `<span class="pill"><span class="dot ${ok?'ok':'err'}"></span>${esc(txt)}</span>`}
async function status(){try{const s=await api('/api/status');
 $('hsub').textContent='v'+s.fw+' · '+(s.profile_name||s.profile)+' · '+(s.wifi.ip||'');if($('fwcur'))$('fwcur').textContent=t('Installiert: v')+s.fw;
 $('vals').innerHTML=s.pids.filter(p=>p.enabled).map(p=>`<div class="val"><div class="l">${esc(p.name)}</div><div class="v">${p.value??'–'}<small>${esc(p.unit)}</small></div><div class="m">${t('aktualisiert')}: ${age(p.age_s)} · ${t('nächste')}: ${p.next_s} s</div>${p.error?`<div class="e">${esc(p.error)}</div>`:''}</div>`).join('')
  +(s.voltage!==undefined?`<div class="val"><div class="l">${t('12V Bordnetz')}</div><div class="v">${s.voltage.toFixed(1)}<small>V</small></div></div>`:'');
 $('conn').innerHTML=pill(s.wifi.connected,'WLAN '+(s.wifi.ssid||'')+(s.wifi.connected?' ('+s.wifi.rssi+' dBm)':''))+pill(s.mqtt,'MQTT')
  +pill(s.ble.connected,'BLE '+(s.ble.name?s.ble.name+(s.ble.mac?' ('+s.ble.mac+')':''):(s.ble.mac||t('nicht konfiguriert')))+(s.ble.connected?' ('+s.ble.rssi+' dBm)':s.ble.next_try_s?' – '+t('Versuch in')+' '+s.ble.next_try_s+' s':''));
 $('lerr').textContent=s.last_error?t('Letzter Fehler: ')+s.last_error:'';$('safebox').classList.toggle('hide',!s.safe_mode);
 const lg=$('log'),atB=lg.scrollTop+lg.clientHeight>=lg.scrollHeight-5;lg.textContent=s.log.join('\n');if(atB)lg.scrollTop=lg.scrollHeight;
 if(s.ble.uuids||s.ble.name)$('uuids').textContent=t('Erkannt: ')+[s.ble.name,s.ble.kind,s.ble.elm].filter(Boolean).join(' · ')+(s.ble.uuids?' (UUIDs '+s.ble.uuids.replace(/0x/g,'')+')':'');
 if($('devtime'))$('devtime').textContent=t('Uhrzeit am Gerät')+': '+(s.time||t('noch nicht synchronisiert'));
 $('sysinfo').innerHTML=`Firmware ${esc(s.fw)}<br>${t('Letzter Neustart')}: ${esc(s.reset_reason||'')}<br>${t('Laufzeit')} ${age(s.uptime)}<br>${t('Freier Heap')} ${Math.round(s.heap/1024)} KB (${t('Minimum')} ${Math.round((s.heap_min||0)/1024)} KB, ${t('größter Block')} ${Math.round((s.heap_block||0)/1024)} KB)<br>${t('Dateisystem')} ${Math.round((s.fs_used||0)/1024)} / ${Math.round((s.fs_total||0)/1024)} KB<br>${t('Abfragezyklen')} ${s.cycles}<br>IP ${esc(s.wifi.ip)} (${s.wifi.ip_mode==='static'?t('fest'):s.wifi.ip_mode==='fallback'?t('DHCP-Fallback – feste IP hat nicht funktioniert'):'DHCP'})`;
 if($('ipfb'))$('ipfb').textContent=s.wifi.ip_mode==='fallback'?t('Die feste IP hat beim letzten Start nicht funktioniert – aktuell per DHCP: ')+s.wifi.ip:'';
}catch(e){}}
async function copyLog(){const tr=t;let txt='';try{txt=await (await fetch('/api/log')).text()}catch(e){txt=$('log').textContent}
 let ok=false;try{if(navigator.clipboard&&window.isSecureContext){await navigator.clipboard.writeText(txt);ok=true}}catch(e){}
 if(!ok){const ta=document.createElement('textarea');ta.value=txt;ta.style.position='fixed';ta.style.opacity='0';document.body.appendChild(ta);ta.select();try{ok=document.execCommand('copy')}catch(e){}ta.remove()}
 toast(ok?tr('Log in die Zwischenablage kopiert')+' ('+txt.split('\n').length+' '+tr('Zeilen')+')':tr('Kopieren nicht möglich – bitte „Herunterladen“ nutzen'))}
async function restore(){const f=$('rsfile').files[0];if(!f)return toast(t('Bitte Sicherungsdatei wählen'));
 let j;try{j=JSON.parse(await f.text())}catch(e){return toast(t('Keine gültige JSON-Datei'))}
 if(j.type!=='obd2mqtt-backup')return toast(t('Das ist keine OBD2MQTT-Sicherung'));
 const c=$('rscfg').checked,p=$('rsprof').checked;if(!c&&!p)return toast(t('Bitte Einstellungen und/oder Profile wählen'));
 const np=Object.keys(j.profiles||{}).length;
 if(!confirm(`${t('Sicherung vom')} ${j.created||'?'} (v${j.fw||'?'}) ${t('wiederherstellen?')}\n\n${c?'• '+t('Einstellungen')+(j.with_secrets?t(' inkl. Passwörter'):t(' (Passwörter bleiben wie sie sind)'))+'\n':''}${p?'• '+np+' '+t('Profil(e) – gleichnamige werden überschrieben')+'\n':''}\n${t('Danach startet das Gerät neu.')}`))return;
 const st=$('rsst'),btn=$('rsbtn');btn.disabled=true;st.textContent=t('Übertrage…');
 const r=await post(`/api/restore?config=${c?1:0}&profiles=${p?1:0}`,j);st.textContent=r.msg;if(!r.ok){btn.disabled=false;return}
 await new Promise(x=>setTimeout(x,5000));
 for(let i=0;i<40;i++){try{await api('/api/status');st.textContent=t('Fertig – Gerät neu gestartet.');toast(t('Sicherung wiederhergestellt'));btn.disabled=false;loadCfg();prof=null;status();return}catch(e){}st.textContent=t('Warte auf Neustart…');await new Promise(x=>setTimeout(x,1500))}
 st.textContent=t('Gerät nicht erreichbar – falls sich das WLAN geändert hat, neue Adresse aufrufen.');btn.disabled=false}
async function pollNow(){const r=await api('/api/pollnow',{method:'POST'});toast(r.msg)}
// ---------- Jobs ----------
async function runJob(b){let r=await post('/api/job',b);if(!r.ok){toast(r.msg);return null}
 for(let i=0;i<120;i++){await new Promise(x=>setTimeout(x,400));const j=await api('/api/job');if(j.state==='done')return j.result||{}}return {ok:false,error:t('Zeitüberschreitung')}}
function q(c){$('tcmd').value=c;termSend()}
async function termSend(){const c=$('tcmd').value.trim();if(!c)return;const o=$('tout');o.textContent+='> '+c+'\n';
 const r=await runJob({type:'raw',cmd:c});if(!r)return;o.textContent+=nl(r.ok?r.raw:t('Fehler: ')+(r.error||'')+(r.raw?'\n'+r.raw:''))+'\n\n';o.scrollTop=o.scrollHeight;$('tcmd').select()}
// ---------- Einstellungen ----------
const CF=['wifi_ssid','wifi_pass','hostname','web_pass','ip_mode','ip_addr','ip_mask','ip_gw','ip_dns1','ip_dns2','mqtt_host','mqtt_port','mqtt_user','mqtt_pass','mqtt_base','ha_prefix','device_name','ble_mac','ble_addr_type','ble_service','ble_notify','ble_write','ble_name','min_voltage','retry_sec','backoff_sec','cmd_timeout_ms','keep_connected','ntp_server','tz','log_persist'];
const NUM=['mqtt_port','min_voltage','retry_sec','backoff_sec','cmd_timeout_ms'];
async function loadCfg(){const c=await api('/api/config');CF.forEach(k=>{if($(k))$(k).value=String(c[k]??'')});
 const sel=$('tzsel'),has=[...sel.options].some(o=>o.value===c.tz);sel.value=has?c.tz:'custom';$('tz').classList.toggle('hide',has);ipModeChange();status()}
function ipModeChange(){$('ipbox').classList.toggle('hide',$('ip_mode').value!=='static')}
async function ipTakeCurrent(){const s=await api('/api/status');if(!s||!s.wifi)return;$('ip_addr').value=s.wifi.ip||'';$('ip_mask').value=s.wifi.mask||'';$('ip_gw').value=s.wifi.gw||'';$('ip_dns1').value=s.wifi.dns||'';toast(t('Übernommen – Speichern nicht vergessen'))}
const isIp=v=>/^(25[0-5]|2[0-4]\d|1?\d?\d)(\.(25[0-5]|2[0-4]\d|1?\d?\d)){3}$/.test(v);
function tzChange(){const v=$('tzsel').value;if(v==='custom'){$('tz').classList.remove('hide');$('tz').focus()}else{$('tz').value=v;$('tz').classList.add('hide')}}
async function saveCfg(rb){if($('ip_mode').value==='static'){for(const k of ['ip_addr','ip_mask','ip_gw'])if(!isIp($(k).value.trim())){toast(t('Ungültige IP-Angabe: ')+$(k).previousElementSibling.textContent);$(k).focus();return}
 for(const k of ['ip_dns1','ip_dns2'])if($(k).value.trim()&&!isIp($(k).value.trim())){toast(t('Ungültige IP-Angabe: ')+$(k).previousElementSibling.textContent);$(k).focus();return}}
 const b={};CF.forEach(k=>{let v=$(k).value;if(NUM.includes(k))v=Number(v);if(k==='keep_connected'||k==='log_persist')v=v==='true';b[k]=v});
 const r=await post('/api/config'+(rb?'?reboot=1':''),b);toast(r.msg)}
async function scan(){const btn=$('scanbtn');btn.disabled=true;btn.textContent=t('Suche läuft (6 s)…');$('scanl').innerHTML='';
 const r=await runJob({type:'scan'});btn.disabled=false;btn.textContent=t('Nach Dongles suchen');if(!r)return;
 $('scanl').innerHTML=(r.devices||[]).map(d=>`<div onclick="pick('${esc(d.mac)}',${d.random},this.dataset.n)" data-n="${esc(d.name||'')}"><span>${d.obd?'★ ':''}<b>${esc(d.name||t('(ohne Namen)'))}</b> <span class="mono">${esc(d.mac)}</span></span><span class="hint" style="margin:0">${d.rssi} dBm</span></div>`).join('')||'<p class="hint">'+t('Keine Geräte gefunden.')+'</p>'}
const bars=r=>r>=-55?'▂▄▆█':r>=-67?'▂▄▆':r>=-75?'▂▄':'▂';
async function wscan(){const btn=$('wscanbtn');btn.disabled=true;btn.textContent=t('Suche…');$('wscanl').innerHTML='';let r=await api('/api/wifiscan?start=1');
 for(let i=0;i<40&&r.state!=='done';i++){await new Promise(x=>setTimeout(x,500));r=await api('/api/wifiscan')}
 btn.disabled=false;btn.textContent=t('Suchen');const n=(r.networks||[]).sort((a,b)=>b.rssi-a.rssi);
 $('wscanl').innerHTML=n.map(w=>`<div data-s="${esc(w.ssid)}" onclick="wpick(this.dataset.s)"><span>${w.open?'':'🔒 '}<b>${esc(w.ssid)}</b> <span class="hint" style="margin:0">${t('Kanal')} ${w.ch}</span></span><span class="hint mono" style="margin:0">${bars(w.rssi)} ${w.rssi} dBm</span></div>`).join('')||'<p class="hint">'+t('Keine Netze gefunden.')+'</p>'}
function wpick(s){$('wifi_ssid').value=s;$('wscanl').innerHTML='';$('wifi_pass').value='';$('wifi_pass').focus();toast(t('Passwort eingeben, dann „Speichern & Neustart“'))}
function pick(m,rnd,n){$('ble_mac').value=m;$('ble_name').value=n||'';$('ble_addr_type').value=rnd?'random':'public';toast(t('Übernommen – Speichern nicht vergessen'))}
// ---------- Profile ----------
const DC=['','battery','voltage','current','power','energy','temperature','distance','speed','duration'];
let pnames={};const pname=id=>pnames[id]||id;
async function loadProfiles(){const r=await api('/api/profiles');active=r.active;pnames=r.names||{};
 const ids=[...r.list].sort((a,b)=>pname(a).localeCompare(pname(b)));
 $('psel').innerHTML=ids.map(n=>`<option value="${esc(n)}" ${n===(profName||active)?'selected':''}>${esc(pname(n))}${n===active?' ✓':''}</option>`).join('');
 await loadProfile(profName&&r.list.includes(profName)?profName:active)}
async function loadProfile(n){profName=n;prof=await api('/api/profile?name='+encodeURIComponent(n));
 $('psel').value=n;$('pact').textContent=n===active?t('Dieses Profil ist aktiv.'):t('Aktiv ist: ')+pname(active);renderProfile()}
function syncAll(){const cs=[...document.querySelectorAll('#pids input[data-k=enabled]')];if($('allpids'))$('allpids').checked=cs.length>0&&cs.every(c=>c.checked)}
function renderProfile(){$('p_name').value=prof.name||'';$('p_model').value=prof.model||'';$('p_init').value=(prof.init||[]).join('\n');
 $('pids').innerHTML=(prof.pids||[]).map((p,i)=>`<tr data-i="${i}">
 <td><input type="checkbox" data-k="enabled" ${p.enabled!==false?'checked':''}></td>
 <td><input data-k="id" value="${esc(p.id)}" style="width:90px"></td><td><input data-k="name" value="${esc(p.name)}" style="width:110px"></td>
 <td><input data-k="header" class="mono" value="${esc(p.header)}" style="width:60px"></td><td><input data-k="cmd" class="mono" value="${esc(p.cmd)}" style="width:80px"></td>
 <td><input data-k="formula" class="mono" value="${esc(p.formula)}" style="width:150px"></td><td><input data-k="unit" value="${esc(p.unit)}" style="width:50px"></td>
 <td><select data-k="device_class">${DC.map(d=>`<option value="${d}" ${d===(p.device_class||'')?'selected':''}>${d||'–'}</option>`).join('')}</select></td>
 <td><input data-k="interval" type="number" value="${p.interval??120}" style="width:70px"></td>
 <td><input data-k="min" type="number" step="any" value="${p.min??''}" style="width:60px"></td><td><input data-k="max" type="number" step="any" value="${p.max??''}" style="width:60px"></td>
 <td><input data-k="precision" type="number" min="0" max="4" value="${p.precision??1}" style="width:48px"></td>
 <td style="white-space:nowrap"><button class="b" onclick="testPid(${i})">Test</button> <button class="b d" onclick="delPid(${i})">✕</button></td></tr>`).join('');syncAll()}
function collect(){prof.name=$('p_name').value;prof.model=$('p_model').value;prof.init=$('p_init').value.split('\n').map(s=>s.trim()).filter(Boolean);
 document.querySelectorAll('#pids tr').forEach(tr=>{const p=prof.pids[tr.dataset.i];tr.querySelectorAll('[data-k]').forEach(el=>{const k=el.dataset.k;
  if(el.type==='checkbox')p[k]=el.checked;else if(el.type==='number'){if(el.value==='')delete p[k];else p[k]=Number(el.value)}else p[k]=el.value.trim()})})}
function addPid(){collect();prof.pids=prof.pids||[];prof.pids.push({id:'neu'+(prof.pids.length+1),name:t('Neu'),header:'7E4',cmd:'220101',formula:'B3',unit:'',interval:120,precision:1,enabled:false});renderProfile()}
function toggleAll(on){document.querySelectorAll('#pids input[data-k=enabled]').forEach(c=>c.checked=on);toast(on?t('Alle Werte aktiviert – Profil speichern nicht vergessen'):t('Alle Werte deaktiviert – Profil speichern nicht vergessen'))}
function delPid(i){collect();prof.pids.splice(i,1);renderProfile()}
async function saveProfile(n){if(!n)collect();const r=await post('/api/profile?name='+encodeURIComponent(n||profName),prof);toast(r.msg);if(r.ok){if(n)profName=n;loadProfiles()}}
function saveAs(){collect();const nm=prompt(t('Name der Kopie:'),(prof.name||profName)+' ('+t('Kopie')+')');if(!nm)return;
 let id=nm.toLowerCase().normalize('NFD').replace(/[\u0300-\u036f]/g,'').replace(/[^a-z0-9]+/g,'_').replace(/^_+|_+$/g,'').slice(0,20)||'profil';
 const base=id;let k=2;while(pnames[id]!==undefined||id===profName){id=(base.slice(0,17)+'_'+k++)}
 prof.name=nm;pnames[id]=nm;saveProfile(id)}
async function activate(){const r=await post('/api/config',{profile:profName});toast(r.ok?t('Profil')+' '+pname(profName)+t(' aktiviert'):r.msg);active=profName;$('pact').textContent=t('Dieses Profil ist aktiv.');loadProfiles()}
async function delProfile(){if(!confirm(t('Profil')+' „'+pname(profName)+'“'+t(' löschen?')))return;const r=await api('/api/profile-delete?name='+encodeURIComponent(profName),{method:'POST'});toast(r.msg);if(r.ok){profName='';loadProfiles()}}
function toggleRaw(){collect();$('rawbox').classList.toggle('hide');$('raw').value=JSON.stringify(prof,null,2)}
function rawApply(){try{prof=JSON.parse($('raw').value);renderProfile();toast(t('Übernommen – jetzt speichern'))}catch(e){toast(t('JSON-Fehler: ')+e.message)}}
async function testPid(i){collect();const p=prof.pids[i];$('testcard').classList.remove('hide');$('testid').textContent=p.id;$('testout').innerHTML='<p class="hint">'+t('Abfrage läuft…')+'</p>';
 const r=await runJob({type:'test',header:p.header,cmd:p.cmd,formula:p.formula});if(!r){$('testout').innerHTML='';return}
 let h='';if(r.value!==undefined)h+=`<div class="val" style="max-width:260px;margin-bottom:10px"><div class="l">${esc(p.formula)}</div><div class="v">${Number(r.value).toFixed(p.precision??1)}<small>${esc(p.unit)}</small></div></div>`;
 if(!r.ok)h+=`<p style="color:var(--err)">${esc(r.error)}</p>`;
 if(r.bytes){h+=`<label>${r.count} Bytes</label><div class="bytes mono">`+r.bytes.split(' ').map((x,j)=>`<span><b>B${j}</b>${x}<b>${parseInt(x,16)}</b></span>`).join('')+'</div>'}
 h+=`<label style="margin-top:10px">${t('Rohantwort')}</label><pre>${esc(nl(r.raw||''))}</pre>`;$('testout').innerHTML=h;$('testcard').scrollIntoView({behavior:'smooth',block:'nearest'})}
// ---------- OTA ----------
const kb=b=>(b/1024).toFixed(0)+' KB';
function ota(){const f=$('fw').files[0];if(!f)return toast(t('Bitte eine Firmware-Datei wählen'));
 if(/factory/i.test(f.name)&&!confirm(f.name+t(' ist die factory.bin (nur für USB).\nFür das Update die *-ota.bin verwenden. Trotzdem hochladen?')))return;
 const oldFw=($('fwcur').textContent.match(/v([\d.]+)/)||[])[1]||'';
 const st=$('otast'),bar=$('otabar'),btn=$('otabtn');btn.disabled=true;bar.classList.remove('hide');bar.value=0;st.textContent=t('Starte…');
 let done=false;
 const poll=async()=>{while(!done){try{const u=await api('/api/update');if(u.state==='running'&&u.total){const p=Math.min(99,Math.round(u.written/u.total*100));bar.value=p;st.textContent=`${t('Schreibe in den Flash')}: ${p} % (${kb(u.written)} / ${kb(u.total)})`}}catch(e){}await new Promise(r=>setTimeout(r,400))}};
 const fd=new FormData();fd.append('firmware',f,f.name);const x=new XMLHttpRequest();x.open('POST','/api/update');
 x.upload.onprogress=e=>{if(bar.value<5)st.textContent=t('Sende Datei…')};
 x.onload=async()=>{done=true;let r={};try{r=JSON.parse(x.responseText)}catch(e){r={ok:false,msg:x.statusText||t('Fehler')}}
  if(!r.ok){st.textContent=r.msg;bar.classList.add('hide');btn.disabled=false;return}
  bar.value=100;st.textContent=t('Fertig – Neustart…');await new Promise(r=>setTimeout(r,4000));
  for(let i=0;i<40;i++){try{const s=await api('/api/status');st.textContent=`${t('Update abgeschlossen')}: v${oldFw} → v${s.fw}`;toast(t('Neue Version läuft: v')+s.fw);btn.disabled=false;status();return}catch(e){}st.textContent=t('Warte auf Neustart…');await new Promise(r=>setTimeout(r,1500))}
  st.textContent=t('ESP32 nicht erreichbar – Seite bitte neu laden');btn.disabled=false};
 x.onerror=()=>{done=true;st.textContent=t('Verbindung abgebrochen');btn.disabled=false};
 x.send(fd);poll()}
api('/api/config').then(c=>{if(c.lang&&c.lang!==LANG)setLang(c.lang,false);if(!c.wifi_ssid)show('cfg')});
const CHANGELOG=[{"v": "0.3.7", "de": ["Einstellungen → WLAN: IP-Adresse automatisch (DHCP) oder fest (IP, Subnetzmaske, Gateway, DNS); Knopf „Aktuelle Werte übernehmen“", "Fallback: Funktioniert die feste IP beim Start nicht (keine WLAN-Verbindung oder MQTT-Broker nicht erreichbar), holt sich die Bridge automatisch eine Adresse per DHCP – Hinweis im Log, unter System und in den Einstellungen", "Ungültige Angaben (z. B. Gateway nicht im Subnetz) werden schon beim Speichern bzw. beim Start erkannt"], "en": ["Settings → WiFi: IP address automatic (DHCP) or static (IP, subnet mask, gateway, DNS); button “Use current values”", "Fallback: if the static IP does not work at startup (no WiFi connection or MQTT broker not reachable), the bridge automatically gets an address via DHCP – shown in the log, under System and in the settings", "Invalid entries (e.g. gateway not in subnet) are detected when saving or at startup"]}, {"v": "0.3.6", "de": ["Fix: Absturzschleife durch volles Dateisystem – die Log-Dateien konnten zusammen ~96 KB von 128 KB belegen; jetzt max. 2 × 16 KB, und das Log belegt nie mehr als 70 % des Flash (Profile/Einstellungen haben Vorrang)", "Fix: Log-Dateien werden nur noch unter Sperre gelesen/geschrieben – Herunterladen konnte mit dem gleichzeitigen Schreiben/Rotieren kollidieren", "Selbstheilung: nach 2 Abstürzen in Folge wird das ältere Log gelöscht, nach 3 auch das aktuelle; ebenso beim Start, wenn das Dateisystem zu voll ist; alte 48-KB-Logs werden beim Update entfernt", "System: freier Heap mit Minimum und größtem Block sowie Belegung des Dateisystems", "Status im sicheren Modus zeigt MQTT-Verbindung wieder korrekt an", "Log-Zeilen direkt nach dem Start haben gleich Ortszeit statt UTC"], "en": ["Fix: crash loop caused by a full file system – the log files could use ~96 KB of 128 KB; now max. 2 × 16 KB, and the log never uses more than 70 % of the flash (profiles/settings take priority)", "Fix: log files are only read/written under a lock – downloading could collide with simultaneous writing/rotation", "Self-healing: after 2 crashes in a row the older log is deleted, after 3 the current one too; same at startup when the file system is too full; old 48 KB logs are removed on update", "System: free heap with minimum and largest block, plus file system usage", "Status in safe mode shows the MQTT connection correctly again", "Log lines right after startup use local time instead of UTC"]}, {"v": "0.3.5", "de": ["MQTT: aktives Fahrzeugprofil unter <base>/profile (+ Details als JSON unter <base>/profile/attributes), in Home Assistant als Sensor „Fahrzeugprofil“", "MQTT: alle Werte inkl. Profil und Zeitstempel als ein JSON unter <base>/state nach jedem Abfragezyklus"], "en": ["MQTT: active vehicle profile at <base>/profile (+ details as JSON at <base>/profile/attributes), in Home Assistant as sensor “Vehicle profile”", "MQTT: all values including profile and timestamp as one JSON at <base>/state after each poll cycle"]}, {"v": "0.3.4", "de": ["Zeitzone als verständliche Auswahlliste (z. B. „Mitteleuropa – Deutschland, Österreich, Schweiz …“) statt POSIX-Angabe; eigene Angabe weiterhin möglich", "Aktuelle Uhrzeit am Gerät wird unter der Zeitzone angezeigt", "Dongle wird mit Namen angezeigt: Bluetooth-Name, erkannter Typ und ELM-Version (z. B. „vLinker MC · vLinker / iOS-Vlink · ELM327 v2.2“), auch im Status und im Log"], "en": ["Time zone as an understandable selection list (e.g. “Central Europe – Germany, Austria, Switzerland …”) instead of a POSIX string; a custom value is still possible", "Current time on the device is shown below the time zone", "Dongle is shown with its name: Bluetooth name, detected type and ELM version (e.g. “vLinker MC · vLinker / iOS-Vlink · ELM327 v2.2”), also in the status and in the log"]}, {"v": "0.3.3", "de": ["Profilauswahl zeigt sprechende Namen statt Dateinamen (z. B. „Hyundai IONIQ 5 / Kia EV6“, „XPeng G9“), sortiert, aktives Profil mit ✓", "„Kopie speichern“ fragt nur noch nach einem Namen – der interne Dateiname wird automatisch erzeugt", "Werksprofile umbenannt: „Hyundai IONIQ 5 / Kia EV6“, „XPeng P7+ (experimentell)“"], "en": ["Profile selection shows readable names instead of file names (e.g. “Hyundai IONIQ 5 / Kia EV6”, “XPeng G9”), sorted, active profile marked with ✓", "“Save copy” only asks for a name – the internal file name is generated automatically", "Factory profiles renamed: “Hyundai IONIQ 5 / Kia EV6”, “XPeng P7+ (experimental)”"]}, {"v": "0.3.2", "de": ["Werksprofile: alle Werte standardmäßig aktiviert (IONIQ 5, G9, G6, P7+)", "Profil-Tabelle: Häkchen im Spaltenkopf „Aktiv“ schaltet alle Werte an/aus"], "en": ["Factory profiles: all values enabled by default (IONIQ 5, G9, G6, P7+)", "Profile table: checkbox in the “Active” column header switches all values on/off"]}, {"v": "0.3.1", "de": ["Neue Werksprofile: XPeng G6 und XPeng P7+ (P7+ experimentell)", "Bestehende Geräte erhalten die neuen Profile automatisch nach dem Update"], "en": ["New factory profiles: XPeng G6 and XPeng P7+ (P7+ experimental)", "Existing devices get the new profiles automatically after the update"]}, {"v": "0.3.0", "de": ["Komplette Oberfläche, Gerätemeldungen und Handbuch auf Deutsch und Englisch – Sprachwahl oben rechts", "Sprache wird im Gerät gespeichert; Log, Fehlermeldungen und Home-Assistant-Namen folgen der Auswahl", "Werksprofile mit deutschen bzw. englischen Wertenamen", "Changelog zweisprachig (CHANGELOG.md / CHANGELOG.en.md)"], "en": ["Complete interface, device messages and manual in German and English – language selector at the top right", "Language is stored on the device; log, error messages and Home Assistant names follow the selection", "Factory profiles with German or English value names", "Bilingual changelog (CHANGELOG.md / CHANGELOG.en.md)"]}, {"v": "0.2.5", "de": ["Einstellungen und Profile sichern (JSON-Datei, wahlweise inkl. Passwörter)", "Sicherung wiederherstellen – Einstellungen und/oder Profile, danach automatischer Neustart"], "en": ["Back up settings and profiles (JSON file, optionally including passwords)", "Restore a backup – settings and/or profiles, followed by an automatic restart"]}, {"v": "0.2.4", "de": ["Benutzerhandbuch direkt im Gerät (Link „Hilfe“ oben rechts bzw. /hilfe)", "Handbuch mit Suchfeld und Live-Prüfung des eigenen Systems mit Links zur passenden Hilfe"], "en": ["User manual built into the device (link “Help” at the top right or /hilfe)", "Manual with search box and live check of your own system with links to the matching help"]}, {"v": "0.2.3", "de": ["Log mit echter Uhrzeit (NTP, Zeitzone einstellbar; vor der Synchronisation Laufzeit „+hh:mm:ss“)", "Log dauerhaft im Flash speichern (übersteht Neustarts, rotiert bei 48 KB), abschaltbar", "Log: Kopieren, Herunterladen, älteres Log herunterladen, Löschen", "Changelog im System-Tab"], "en": ["Log with real time of day (NTP, configurable time zone; before sync the uptime “+hh:mm:ss”)", "Store log permanently in flash (survives restarts, rotates at 48 KB), can be disabled", "Log: copy, download, download older log, clear", "Changelog in the System tab"]}, {"v": "0.2.2", "de": ["Aufwärmphase nach Dongle-Reset (ATRV + Pause) – behebt CAN ERROR beim Test-Button", "Einmalige Wiederholung bei CAN ERROR", "BLE bleibt nach Test/Terminal 60 s verbunden"], "en": ["Warm-up after dongle reset (ATRV + pause) – fixes CAN ERROR on the Test button", "Single retry on CAN ERROR", "BLE stays connected for 60 s after Test/Terminal"]}, {"v": "0.2.1", "de": ["Verspätete ELM-Prompts („>“) werden ignoriert – behebt abgeschnittene Antworten", "Vor jedem Befehl warten, bis der Dongle still ist", "Bis zu 2 Wiederholungen bei unvollständiger Antwort, Rohantwort im Log"], "en": ["Late ELM prompts (“>”) are ignored – fixes truncated responses", "Wait until the dongle is quiet before each command", "Up to 2 retries on incomplete responses, raw response in the log"]}, {"v": "0.2.0", "de": ["Watchdog: Hänger > 60 s, WLAN 15 min bzw. MQTT 20 min weg → Neustart", "Sicherer Modus nach 3 Abstürzen in Folge (ohne BLE, Web-UI erreichbar)", "Grund des letzten Neustarts im System-Tab"], "en": ["Watchdog: hang > 60 s, WiFi 15 min or MQTT 20 min down → restart", "Safe mode after 3 crashes in a row (no BLE, web UI reachable)", "Reason of the last restart in the System tab"]}, {"v": "0.1.10", "de": ["Fix: „Werksprofile wiederherstellen“ und „Profil löschen“ meldeten „Ungültiger Profilname“"], "en": ["Fix: “Restore factory profiles” and “Delete profile” reported “Invalid profile name”"]}, {"v": "0.1.9", "de": ["Unvollständige Multi-Frame-Antworten werden erkannt und wiederholt", "IONIQ 5: Init-Befehl ATST96 (längerer Timeout)", "Klarere Log-Meldung bei planmäßiger BLE-Trennung"], "en": ["Incomplete multi-frame responses are detected and retried", "IONIQ 5: init command ATST96 (longer timeout)", "Clearer log message for scheduled BLE disconnects"]}, {"v": "0.1.8", "de": ["Status-LED entfernt (ESP32-DevKitC V4 hat keine steuerbare LED)"], "en": ["Status LED removed (ESP32-DevKitC V4 has no controllable LED)"]}, {"v": "0.1.7", "de": ["Firmware-Update mit echtem Fortschritt, Fehlermeldungen und Warten auf Neustart", "Prüfung auf factory.bin, BLE-Abfragen pausieren während des Updates"], "en": ["Firmware update with real progress, error messages and waiting for the restart", "Check for factory.bin, BLE polling pauses during the update"]}, {"v": "0.1.5 – 0.1.6", "de": ["Versionsnummer im Dateinamen und in der Weboberfläche", "LED-Test (später entfernt)"], "en": ["Version number in the file name and in the web interface", "LED test (removed later)"]}, {"v": "0.1.4", "de": ["XPeng G9: SoH 22110A, Kilometerstand 3 Bytes, CLTC-Reichweite 221118, Ladelimit/Ladestatus"], "en": ["XPeng G9: SoH 22110A, odometer 3 bytes, CLTC range 221118, charge limit/charge status"]}, {"v": "0.1.3", "de": ["Batterietemperatur, Kilometerstand, berechnete Reichweite; G9-Profil nach WiCAN"], "en": ["Battery temperature, odometer, calculated range; G9 profile based on WiCAN"]}, {"v": "0.1.1", "de": ["WLAN-Suche in den Einstellungen"], "en": ["WiFi scan in the settings"]}, {"v": "0.1.0", "de": ["Erste Version: BLE-ELM327 → MQTT mit Home-Assistant-Discovery, Weboberfläche, Profile, Test, Terminal, OTA"], "en": ["First version: BLE ELM327 → MQTT with Home Assistant discovery, web interface, profiles, test, terminal, OTA"]}];
function renderChangelog(){$('changelog').innerHTML=CHANGELOG.map((c,i)=>`<div style="margin-bottom:10px"><b>v${esc(c.v)}</b>${i==0?' <span class="pill" style="padding:0 7px;font-size:11px">'+t('neu')+'</span>':''}<ul style="margin:4px 0 0 18px;padding:0">${(LANG==='en'?c.en:c.de).map(x=>`<li>${esc(x)}</li>`).join('')}</ul></div>`).join('')}
applyLang();
status();setInterval(()=>{if(cur==='status'||cur==='sys'||cur==='cfg')status()},3000);
</script></body></html>)HTML";
