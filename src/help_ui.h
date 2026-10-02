#pragma once
#include <Arduino.h>

// Benutzer-Handbuch (Deutsch / English) – liegt im Flash, ausgeliefert unter /hilfe bzw. /hilfe?lang=en
static const char HELP_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="de"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>OBD2MQTT – Handbuch</title>
<style>
:root{--bg:#f4f5f7;--card:#fff;--fg:#1d2330;--mut:#6b7385;--bd:#dfe3ea;--acc:#1f6feb;--ok:#1a7f37;--err:#cf222e;--warn:#9a6700;--code:#eef1f5}
@media(prefers-color-scheme:dark){:root{--bg:#0f1218;--card:#181c24;--fg:#e6e9ef;--mut:#8b93a5;--bd:#2a303c;--acc:#4c8dff;--ok:#3fb950;--err:#f85149;--warn:#d29922;--code:#11151c}}
*{box-sizing:border-box}body{margin:0;font:16px/1.6 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;background:var(--bg);color:var(--fg)}
header{background:var(--card);border-bottom:1px solid var(--bd);padding:14px 16px;position:sticky;top:0;z-index:2}
header h1{margin:0;font-size:18px}header .sub{color:var(--mut);font-size:13px}
header a{color:var(--acc);text-decoration:none;font-size:14px;float:right;margin-top:4px}
main{max-width:860px;margin:0 auto;padding:16px}
section{background:var(--card);border:1px solid var(--bd);border-radius:12px;padding:6px 20px 14px;margin-bottom:16px}
h2{font-size:19px;margin:16px 0 8px}h3{font-size:16px;margin:16px 0 6px}
a{color:var(--acc)}code,kbd{font-family:ui-monospace,Menlo,Consolas,monospace;font-size:.88em;background:var(--code);border:1px solid var(--bd);border-radius:5px;padding:1px 5px}
table{border-collapse:collapse;width:100%;font-size:14.5px;margin:8px 0}th,td{border-bottom:1px solid var(--bd);padding:6px 6px;text-align:left;vertical-align:top}th{color:var(--mut);font-weight:500}
.toc{columns:2;column-gap:24px;padding-left:18px}@media(max-width:600px){.toc{columns:1}}
.box{border-left:4px solid var(--acc);background:var(--code);padding:8px 12px;border-radius:0 8px 8px 0;margin:10px 0}
.box.warn{border-color:var(--warn)}.box.ok{border-color:var(--ok)}
ol li,ul li{margin:3px 0}
details{border:1px solid var(--bd);border-radius:8px;padding:6px 12px;margin:8px 0}summary{cursor:pointer;font-weight:600}
#q{width:100%;margin-top:10px;font:inherit;padding:8px 11px;border:1px solid var(--bd);border-radius:8px;background:var(--bg);color:var(--fg)}
mark{background:#ffe58a;color:#000;border-radius:3px;padding:0 1px}
.chk{display:flex;gap:10px;align-items:flex-start;padding:7px 0;border-bottom:1px solid var(--bd)}.chk:last-child{border:0}
.dot{flex:none;width:10px;height:10px;border-radius:50%;margin-top:7px;background:var(--mut)}.dot.ok{background:var(--ok)}.dot.err{background:var(--err)}.dot.warn{background:var(--warn)}
.hide{display:none!important}.nores{color:var(--mut);text-align:center;padding:20px}
</style></head><body>
<header><a href="/">← zurück zur Oberfläche</a><a href="/hilfe?lang=en" style="margin-right:14px">🇬🇧 English</a><h1>OBD2MQTT – Handbuch</h1><div class="sub" id="ver">Bedienungsanleitung für Anwender</div>
<input id="q" type="search" placeholder="Im Handbuch suchen … (z. B. „NO DATA“, „Update“, „Passwort“)" oninput="search(this.value)"></header>
<main>
<p class="nores hide" id="nores">Nichts gefunden – anderen Begriff probieren.</p>

<section id="check">
<h2>Dein System gerade</h2>
<p style="margin:0 0 6px;color:var(--mut);font-size:14px">Live-Prüfung deines Geräts – mit Link zur passenden Hilfe.</p>
<div id="checks"><div class="chk"><span class="dot"></span><span>Lade Status …</span></div></div>
</section>

<section id="inhalt">
<h2>Inhalt</h2>
<ol class="toc">
<li><a href="#was">Was macht das Gerät?</a></li>
<li><a href="#brauche">Was brauche ich?</a></li>
<li><a href="#install">Erstinstallation</a></li>
<li><a href="#einrichten">Einrichtung Schritt für Schritt</a></li>
<li><a href="#bedienung">Die Oberfläche</a></li>
<li><a href="#ha">Werte in Home Assistant</a></li>
<li><a href="#werte">Welche Werte gibt es?</a></li>
<li><a href="#eigene">Eigene Werte hinzufügen</a></li>
<li><a href="#update">Firmware aktualisieren</a></li>
<li><a href="#backup">Sichern &amp; Wiederherstellen</a></li>
<li><a href="#akku">12-V-Batterie &amp; Stromsparen</a></li>
<li><a href="#fehler">Probleme lösen</a></li>
<li><a href="#begriffe">Begriffe kurz erklärt</a></li>
</ol>
</section>

<section id="was">
<h2>1. Was macht das Gerät?</h2>
<p>Der kleine ESP32 in deiner Garage verbindet sich per <b>Bluetooth</b> mit dem OBD2-Stecker in deinem Auto, liest Daten wie <b>Ladestand (SoC)</b>, <b>Batterietemperatur</b> und <b>Kilometerstand</b> aus und schickt sie per <b>MQTT</b> an <b>Home Assistant</b>.</p>
<p>So sieht das zusammen aus:</p>
<div class="box">Auto (OBD2-Dongle) ⟶ <i>Bluetooth</i> ⟶ ESP32 in der Garage ⟶ <i>WLAN</i> ⟶ MQTT-Broker ⟶ Home Assistant</div>
<p>Sobald das Auto in Reichweite steht (typisch 5–15 m) und wach ist – z. B. beim Laden – kommen die Werte automatisch. Ist das Auto weg oder schläft, bleibt in Home Assistant der letzte Wert stehen.</p>
</section>

<section id="brauche">
<h2>2. Was brauche ich?</h2>
<ul>
<li>Einen <b>ESP32</b> (z. B. ESP32-DevKitC) mit USB-Netzteil in Bluetooth-Reichweite des Autos.</li>
<li>Einen <b>Bluetooth-LE-OBD2-Dongle</b> (ELM327-kompatibel, „BLE“ bzw. „Bluetooth 4.0“), z. B. Vgate iCar Pro BLE, vLinker MC+/FS, Veepeak BLE+, OBDLink CX. <i>Klassische Bluetooth-Dongles (ohne „LE“) funktionieren nicht.</i></li>
<li>WLAN in der Garage.</li>
<li>Einen <b>MQTT-Broker</b> (z. B. das Mosquitto-Add-on in Home Assistant) mit Benutzer und Passwort.</li>
<li>Home Assistant mit eingerichteter <b>MQTT-Integration</b>.</li>
</ul>
</section>

<section id="install">
<h2>3. Erstinstallation (nur einmal, per USB)</h2>
<ol>
<li>ESP32 per USB-Kabel an den Computer anschließen.</li>
<li>In <b>Chrome</b> oder <b>Edge</b> die Seite <a href="https://espressif.github.io/esptool-js/" target="_blank">espressif.github.io/esptool-js</a> öffnen.</li>
<li><b>Connect</b> klicken (Baudrate 460800) und den USB-Port auswählen. Klappt es nicht: beim Verbinden die <kbd>BOOT</kbd>-Taste am ESP32 gedrückt halten.</li>
<li><b>Erase Flash</b> klicken (löscht alte Firmware).</li>
<li>Datei <code>obd2mqtt-vX.Y.Z-esp32-<b>factory</b>.bin</code> wählen, Adresse <code>0x0</code>, <b>Program</b> klicken.</li>
<li>Nach „Done“ den ESP32 kurz vom Strom nehmen und wieder anschließen.</li>
</ol>
<div class="box warn">Die <b>factory.bin</b> ist nur für USB. Für spätere Updates über die Oberfläche nimmst du die <b>ota.bin</b> (siehe <a href="#update">Kapitel 9</a>).</div>
</section>

<section id="einrichten">
<h2>4. Einrichtung Schritt für Schritt</h2>
<h3>4.1 Mit dem Gerät verbinden</h3>
<ol>
<li>Mit Handy oder Laptop das WLAN <code>OBD2MQTT-xxxx</code> verbinden – Passwort <code>obd2mqtt</code>.</li>
<li>Meist öffnet sich die Einrichtungsseite von selbst, sonst im Browser <code>http://192.168.4.1</code> aufrufen.</li>
<li>Oben rechts die Sprache wählen (🇩🇪 DE / 🇬🇧 EN).</li>
</ol>
<h3>4.2 WLAN</h3>
<ol>
<li>Reiter <b>Einstellungen</b> → bei „SSID“ auf <b>Suchen</b> klicken und dein WLAN auswählen.</li>
<li>WLAN-Passwort eingeben.</li>
</ol>
<h3>4.3 MQTT</h3>
<p>Broker-Adresse (IP deines Home Assistant bzw. MQTT-Servers), Port <code>1883</code>, Benutzer und Passwort eintragen. „Gerätename in HA“ ist der Name, unter dem das Auto in Home Assistant erscheint (z. B. „IONIQ 5“).</p>
<h3>4.4 Dongle koppeln</h3>
<ol>
<li>Auto mit eingestecktem Dongle in die Nähe stellen.</li>
<li>Bei <b>BLE-Dongle</b> auf <b>Nach Dongles suchen</b> klicken (dauert 6 s).</li>
<li>Den passenden Eintrag anklicken – mit ★ markierte sind wahrscheinlich OBD-Dongles. Die UUID-Felder leer lassen, die werden automatisch erkannt. Nach der ersten Verbindung steht unter dem Knopf, welcher Dongle erkannt wurde (Name, Typ, ELM-Version).</li>
</ol>
<h3>4.5 Speichern</h3>
<p><b>Speichern &amp; Neustart</b> klicken. Der ESP32 verbindet sich jetzt mit deinem WLAN. Danach erreichst du ihn unter <code>http://obd2mqtt.local</code> oder unter der IP-Adresse, die dein Router vergeben hat (steht in der Geräteliste deines Routers).</p>
<h3>4.6 Fahrzeugprofil wählen</h3>
<ol>
<li>Reiter <b>Profil</b> → passendes Profil auswählen (z. B. „Hyundai IONIQ 5 / Kia EV6“ oder „XPeng G9“) → <b>Aktivieren</b>.</li>
<li>Auto einschalten oder laden lassen, dann bei „soc“ auf <b>Test</b> klicken. Erscheint ein plausibler Wert, ist alles eingerichtet.</li>
</ol>
<div class="box ok">Fertig! Im Reiter <b>Status</b> siehst du ab jetzt die aktuellen Werte, in Home Assistant erscheint ein neues Gerät.</div>
</section>

<section id="bedienung">
<h2>5. Die Oberfläche</h2>
<table>
<tr><th>Reiter</th><th>Wofür?</th></tr>
<tr><td><b>Status</b></td><td>Aktuelle Werte, Verbindungsstatus (WLAN, MQTT, Bluetooth), Log. <b>Jetzt abfragen</b> holt sofort neue Werte. Beim Log: <b>Kopieren</b>, <b>Herunterladen</b>, <b>Löschen</b>.</td></tr>
<tr><td><b>Profil</b></td><td>Welche Werte abgefragt werden, wie oft (Spalte „Intervall s“) und wie sie berechnet werden. Mit <b>Test</b> lässt sich jeder Wert sofort ausprobieren.</td></tr>
<tr><td><b>Terminal</b></td><td>Für Fortgeschrittene: Befehle direkt an den Dongle schicken.</td></tr>
<tr><td><b>Einstellungen</b></td><td>WLAN, MQTT, Dongle, Abfrage-Zeiten, Uhrzeit, Passwort für die Oberfläche.</td></tr>
<tr><td><b>System</b></td><td>Version, Grund des letzten Neustarts, Neustart, Werksprofile, Sichern &amp; Wiederherstellen, Changelog, Firmware-Update.</td></tr>
<tr><td><b>🇩🇪/🇬🇧</b></td><td>Sprache der Oberfläche, der Gerätemeldungen und der Namen in Home Assistant. Die Auswahl wird im Gerät gespeichert.</td></tr>
</table>
<h3>Wichtige Einstellungen</h3>
<table>
<tr><th>Einstellung</th><th>Bedeutung</th><th>Standard</th></tr>
<tr><td>Abfrage EIN/AUS</td><td>Hauptschalter auf der Status-Seite (und in Home Assistant als Schalter „Abfrage“). Bei AUS verbindet sich die Bridge nicht mehr mit dem Auto – z. B. in der Werkstatt oder bei langer Standzeit. Test, Terminal und „Jetzt abfragen“ funktionieren weiter. Bleibt nach einem Neustart erhalten.</td><td>EIN</td></tr>
<tr><td>12V-Mindestspannung</td><td>Unter diesem Wert wird nicht abgefragt, um die 12-V-Batterie zu schonen.</td><td>12,2 V</td></tr>
<tr><td>Auto schläft unter</td><td>Liegt die 12-V-Spannung darunter, ist das Auto aus und lädt nicht. Dann schickt die Bridge <b>keine Anfragen ans Auto</b> (die würden es jedes Mal aufwecken) und misst nur einmal pro Minute die Spannung am Dongle. Sobald das Auto selbst aufwacht (Fahren, Laden, Vorklimatisieren, 12-V-Nachladung), wird wieder normal abgefragt. Ein wacher IONIQ 5 liegt bei ca. 14,7 V, ein schlafender bei 12,4–12,8 V. Bei einer 12-V-Lithiumbatterie (Ruhespannung ~13,3 V) den Wert auf ca. 13,6 V erhöhen. 0 = immer abfragen.</td><td>13,2 V</td></tr>
<tr><td>Abfrage, während das Auto schläft</td><td>Trotz Schlaf alle N Minuten einmal abfragen (weckt das Auto kurz). 0 = nie. „Jetzt abfragen“ fragt immer ab.</td><td>0</td></tr>
<tr><td>Neuer Verbindungsversuch nach</td><td>Wartezeit, wenn der Dongle nicht erreichbar ist (Auto weg).</td><td>60 s</td></tr>
<tr><td>Pause wenn Auto nicht antwortet</td><td>Wartezeit, wenn das Auto schläft.</td><td>600 s</td></tr>
<tr><td>BLE-Verbindung dauerhaft halten</td><td>„nein“ = Dongle darf zwischen den Abfragen schlafen (empfohlen).</td><td>nein</td></tr>
<tr><td>IP-Adresse</td><td>„automatisch (DHCP)“ oder eine feste Adresse. Mit „Aktuelle Werte übernehmen“ werden IP, Maske, Gateway und DNS der laufenden Verbindung eingetragen. Die Adresse muss außerhalb des DHCP-Bereichs des Routers liegen. Klappt die feste IP nach dem Neustart nicht (kein WLAN oder MQTT-Broker nicht erreichbar), nimmt die Bridge automatisch DHCP und meldet das im Log und unter System.</td><td>DHCP</td></tr>
<tr><td>Web-Passwort</td><td>Schützt die Oberfläche. Anmeldung mit Benutzer <code>admin</code>.</td><td>leer</td></tr>
<tr><td>Zeitzone</td><td>Für die Uhrzeit im Log. Aus der Liste wählen; darunter wird die aktuelle Uhrzeit am Gerät angezeigt.</td><td>Mitteleuropa</td></tr>
<tr><td>Log dauerhaft speichern</td><td>Log übersteht Neustarts – hilfreich bei der Fehlersuche.</td><td>ja</td></tr>
</table>
</section>

<section id="ha">
<h2>6. Werte in Home Assistant</h2>
<p>Die Werte erscheinen <b>automatisch</b> – du musst nichts in Home Assistant konfigurieren. Unter <i>Einstellungen → Geräte &amp; Dienste → MQTT</i> findest du ein Gerät mit dem eingestellten Namen und u. a. diesen Sensoren:</p>
<table>
<tr><th>Sensor</th><th>Bedeutung</th></tr>
<tr><td>SoC</td><td>Ladestand der Antriebsbatterie in %</td></tr>
<tr><td>Batterie Temp max / min</td><td>Höchste/niedrigste Temperatur in der Batterie</td></tr>
<tr><td>Kilometerstand</td><td>Gesamt-km</td></tr>
<tr><td>12V Bordnetz</td><td>Spannung der 12-V-Batterie</td></tr>
<tr><td>OBD-Dongle verbunden</td><td>„Verbunden“, wenn das Auto zuletzt erreichbar war</td></tr>
<tr><td>Fahrzeugprofil</td><td>Name des aktiven Profils (Details als Attribute)</td></tr>
<tr><td>BLE Signal / Letzter Fehler</td><td>Diagnose</td></tr>
</table>
<p>Den SoC kannst du z. B. in <b>evcc</b> als Fahrzeug-SoC nutzen oder für Automationen („Benachrichtige mich bei 80 %“).</p>
<p>Für eigene Auswertungen schickt das Gerät nach jeder Abfrage zusätzlich alle Werte samt Profil als ein JSON an <code>obd2mqtt/state</code>, z. B. <code>{"profile":"XPeng G9","values":{"soc":71.5,…}}</code>.</p>
</section>

<section id="werte">
<h2>7. Welche Werte gibt es?</h2>
<p>Im Reiter <b>Profil</b> kannst du einzelne Werte per Häkchen in der Spalte „Aktiv“ ein- und ausschalten – das Häkchen im Spaltenkopf schaltet alle auf einmal – und dann <b>Profil speichern</b>. In den Werksprofilen sind alle Werte aktiv.</p>
<h3>Hyundai IONIQ 5 (auch Kia EV6, IONIQ 6)</h3>
<p>Alle Werte sind aktiv: SoC, Batterietemperatur max/min, Kilometerstand, SoH (Batteriegesundheit), roher BMS-SoC und berechnete Reichweite.</p>
<h3>XPeng G9, G6 und P7+</h3>
<p>Je ein eigenes Profil (<code>g9</code>, <code>g6</code>, <code>p7plus</code>) mit denselben Werten, alle aktiv: SoC, Batterietemperatur max/min, Kilometerstand, Reichweite (CLTC), SoH, Ladelimit, Ladestatus, HV-Spannung/-Strom, berechnete Reichweite und der Standard-OBD2-SoC.</p>
<p>Am besten geprüft ist der <b>G6</b>. Der G9 nutzt laut Nutzerberichten dieselben Befehle. Für den <b>P7+</b> gibt es noch keine veröffentlichten Werte – das Profil ist ein Versuch auf Basis von G6/G9.</p>
<div class="box warn">Die XPeng-Werte stammen aus der Community und sind nicht für jedes Modelljahr geprüft – vergleiche sie einmal mit der Anzeige im Auto.</div>
<h3>Reichweite</h3>
<p>Die Reichweite aus dem Cockpit lässt sich nicht auslesen. „Reichweite (berechnet)“ schätzt sie aus SoC × Batteriekapazität ÷ Verbrauch. Passe die Formel an dein Auto an, z. B. <code>B34/2/100*63/0.17</code> für 63 kWh und 17 kWh/100 km. Beim XPeng ist zusätzlich die „CLTC-Reichweite“ verfügbar – die ist meist 15–20 % optimistischer als WLTP.</p>
<h3>Wie oft wird abgefragt?</h3>
<p>Spalte <b>Intervall s</b> im Profil: SoC alle 120 s, Temperaturen alle 300 s, Kilometerstand alle 1800 s. Werte mit gleichem Befehl am besten mit gleichem Intervall abfragen.</p>
<p>Hinweis: Die Wertenamen der Werksprofile richten sich nach der Sprache, die beim Einspielen aktiv ist. Nach einem Sprachwechsel benennt <b>Werksprofile wiederherstellen</b> (Reiter System) sie um.</p>
</section>

<section id="eigene">
<h2>8. Eigene Werte hinzufügen (für Neugierige)</h2>
<ol>
<li>Profil → <b>+ PID</b>.</li>
<li><b>Header</b> (Steuergerät, z. B. <code>7E4</code>) und <b>Befehl</b> (z. B. <code>220101</code>) eintragen – solche Angaben findest du in Foren oder Projekten wie evDash, WiCAN oder OVMS.</li>
<li>Als Formel erst einmal <code>B3</code> eintragen und <b>Test</b> drücken. Du siehst alle empfangenen Bytes mit Nummer (B0, B1, …).</li>
<li>Formel anpassen, bis der Wert stimmt. Hilfsmittel: <code>u16(B3,B4)</code> = 2-Byte-Zahl, <code>s8(B5)</code> = Zahl mit Vorzeichen, <code>bit(B6,2)</code> = einzelnes Bit.</li>
<li>Einheit, Klasse und Intervall setzen, Häkchen bei „Aktiv“ und <b>Profil speichern</b>. Home Assistant legt den Sensor automatisch an.</li>
</ol>
<p>Tipp: Mit <b>Kopie speichern…</b> legst du vorher eine Sicherung deines Profils an – du vergibst dabei einfach einen Namen, z. B. „Mein G9“. Den Anzeigenamen eines Profils änderst du im Feld <b>Name</b>.</p>
</section>

<section id="update">
<h2>9. Firmware aktualisieren</h2>
<ol>
<li>Reiter <b>System</b> → bei „Firmware-Update“ die Datei <code>obd2mqtt-vX.Y.Z-esp32-<b>ota</b>.bin</code> auswählen.</li>
<li><b>Hochladen</b> klicken. Der Balken zeigt den Fortschritt, danach startet der ESP32 neu und die Seite zeigt die neue Version an.</li>
</ol>
<p>Einstellungen und Profile bleiben dabei erhalten. Was sich geändert hat, steht im <b>Changelog</b> im Reiter System. Neue Werksprofile übernimmst du mit <b>Werksprofile wiederherstellen</b> (überschreibt eigene Änderungen an <code>ioniq5</code>/<code>g9</code>).</p>
</section>

<section id="backup">
<h2>9a. Sichern &amp; Wiederherstellen</h2>
<p>Im Reiter <b>System</b> → „Sichern &amp; Wiederherstellen“:</p>
<ul>
<li><b>Sicherung herunterladen</b> speichert alle Einstellungen und Fahrzeugprofile als Datei <code>obd2mqtt-backup-DATUM.json</code>.</li>
<li>Häkchen <b>inkl. Passwörter</b>: Nur setzen, wenn du die Datei sicher aufbewahrst – sie enthält dann WLAN-, MQTT- und Web-Passwort im Klartext.</li>
<li><b>Wiederherstellen</b>: Datei wählen, auswählen ob <b>Einstellungen</b> und/oder <b>Profile</b> übernommen werden, bestätigen. Das Gerät startet danach neu.</li>
<li>Bei einer Sicherung ohne Passwörter bleiben die aktuell gespeicherten Passwörter erhalten.</li>
</ul>
<div class="box">Tipp: Vor einem Neu-Flashen mit „Erase Flash“ immer eine Sicherung inkl. Passwörter ziehen – danach ist das Gerät in einer Minute wieder eingerichtet (Setup-WLAN verbinden → System → Wiederherstellen).</div>
</section>

<section id="akku">
<h2>10. 12-V-Batterie &amp; Stromsparen</h2>
<ul>
<li>Der Dongle steckt dauerhaft im Auto und braucht etwas Strom. Nimm einen Dongle mit <b>automatischem Schlafmodus</b>.</li>
<li>Die Firmware trennt Bluetooth zwischen den Abfragen, damit der Dongle schlafen kann.</li>
<li>Antwortet das Auto nicht (es schläft), wird 10 Minuten pausiert.</li>
<li>Fällt die 12-V-Spannung unter die Mindestspannung, werden die Abfragen pausiert.</li>
<li>Wenn du länger nicht fährst (Urlaub): Dongle abziehen.</li>
</ul>
</section>

<section id="fehler">
<h2>11. Probleme lösen</h2>
<details><summary>„NO DATA“ oder „CAN ERROR“</summary>
<p>Das Auto schläft. Die Steuergeräte antworten nur, wenn das Auto eingeschaltet ist oder lädt. Das ist normal – der letzte Wert bleibt in Home Assistant erhalten. Zum Testen: Auto einschalten oder Ladekabel anstecken.</p></details>
<details><summary>„Dongle nicht erreichbar“</summary>
<p>Auto nicht in Reichweite, Dongle schläft oder die Bluetooth-Verbindung ist zu schwach. „BLE Signal“ sollte besser als etwa −85 dBm sein – sonst ESP32 näher ans Auto stellen. Wichtig: Der Dongle darf nicht gleichzeitig mit einer Handy-App verbunden sein.</p></details>
<details><summary>Es kommen keine Werte in Home Assistant an</summary>
<p>Im Reiter Status prüfen, ob „MQTT“ grün ist. Falls nicht: Broker-Adresse, Benutzer und Passwort prüfen. In Home Assistant muss die MQTT-Integration eingerichtet sein.</p></details>
<details><summary>„Unvollständige Antwort“ im Log</summary>
<p>Der Dongle hat eine lange Antwort nicht vollständig geliefert. Die Firmware wiederholt dann automatisch. Kommt es häufig vor: im Profil bei den Init-Befehlen <code>ATST96</code> ergänzen.</p></details>
<details><summary>Ein Wert ist offensichtlich falsch</summary>
<p>Im Profil beim Wert auf <b>Test</b> klicken und mit der Anzeige im Auto vergleichen. Oft stimmt nur die Byte-Nummer in der Formel nicht (siehe <a href="#eigene">Kapitel 8</a>).</p></details>
<details><summary>Oberfläche nicht erreichbar</summary>
<p>ESP32 kurz vom Strom nehmen. Ist das WLAN 3 Minuten nicht erreichbar, öffnet er wieder das Setup-WLAN <code>OBD2MQTT-xxxx</code>. Hängt die Firmware länger als 60 s, startet sie von selbst neu.</p></details>
<details><summary>Roter Hinweis „Sicherer Modus“</summary>
<p>Die Firmware ist mehrmals hintereinander abgestürzt und läuft deshalb ohne Bluetooth. Prüfe zuletzt geänderte Einstellungen bzw. das Profil und klicke dann <b>Normal neu starten</b>. Das gespeicherte Log zeigt, was vorher passiert ist.</p></details>
<details><summary>Ich habe das Web-Passwort vergessen</summary>
<p>Per USB mit <b>Erase Flash</b> neu installieren (<a href="#install">Kapitel 3</a>). Danach sind alle Einstellungen gelöscht – mit einer <a href="#backup">Sicherung</a> (ohne Passwörter) ist alles andere schnell wieder da.</p></details>
<details><summary>Hilfe anfragen</summary>
<p>Im Reiter Status beim Log auf <b>Kopieren</b> klicken und den Text zusammen mit der Version (oben links) weitergeben.</p></details>
</section>

<section id="begriffe">
<h2>12. Begriffe kurz erklärt</h2>
<table>
<tr><td><b>OBD2</b></td><td>Diagnose-Stecker im Auto, meist im Fußraum links.</td></tr>
<tr><td><b>Dongle / ELM327</b></td><td>Der kleine Adapter im OBD2-Stecker, der die Daten per Bluetooth bereitstellt.</td></tr>
<tr><td><b>BLE</b></td><td>Bluetooth Low Energy – stromsparendes Bluetooth.</td></tr>
<tr><td><b>SoC / SoH</b></td><td>State of Charge (Ladestand) / State of Health (Batteriegesundheit).</td></tr>
<tr><td><b>PID</b></td><td>Ein abfragbarer Wert bzw. der Befehl dafür.</td></tr>
<tr><td><b>Header</b></td><td>Adresse des Steuergeräts im Auto, das gefragt wird (z. B. 7E4 = Batteriemanagement beim IONIQ 5).</td></tr>
<tr><td><b>MQTT</b></td><td>Nachrichten-Protokoll, über das Home Assistant die Werte empfängt.</td></tr>
<tr><td><b>OTA</b></td><td>„Over the Air“ – Firmware-Update übers WLAN.</td></tr>
<tr><td><b>RSSI</b></td><td>Signalstärke in dBm. −60 ist sehr gut, −90 sehr schwach.</td></tr>
</table>
</section>
</main>
<script>
const $=id=>document.getElementById(id);
const esc=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
// ---------- Live-Prüfung ----------
function chk(state,text,link){return `<div class="chk"><span class="dot ${state}"></span><span>${text}${link?` – <a href="#${link}" onclick="openSec('${link}')">Hilfe</a>`:''}</span></div>`}
function openSec(id){search('');$('q').value='';const d=document.querySelector('#'+id+' details');if(d)d.open=true}
async function check(){try{const s=await (await fetch('/api/status')).json();
 $('ver').textContent='Bedienungsanleitung · Firmware v'+s.fw;
 let h='';
 h+=s.safe_mode?chk('err','<b>Sicherer Modus aktiv</b> – Bluetooth ist aus.','fehler'):'';
 h+=s.wifi.connected?chk('ok',`WLAN verbunden mit <b>${esc(s.wifi.ssid)}</b> (${s.wifi.rssi} dBm)`):chk('err','Kein WLAN – Einrichtung prüfen.','einrichten');
 h+=s.mqtt?chk('ok','MQTT verbunden – Werte gehen an Home Assistant.'):chk('err','MQTT nicht verbunden – keine Werte in Home Assistant.','fehler');
 if(!s.ble.mac)h+=chk('warn','Noch kein Dongle eingerichtet.','einrichten');
 else if(s.ble.connected)h+=chk('ok',`Dongle verbunden (${s.ble.rssi} dBm)`);
 else h+=chk(s.ble.last_ok_s>=0?'ok':'warn',`Dongle ${esc(s.ble.mac)} gerade nicht verbunden${s.ble.last_ok_s>=0?' – zuletzt erreichbar vor '+fmt(s.ble.last_ok_s)+' (normal: Bluetooth wird zwischen den Abfragen getrennt)':' – noch nie erreicht'}.`,s.ble.last_ok_s>=0?'':'fehler');
 const act=(s.pids||[]).filter(p=>p.enabled);
 const got=act.filter(p=>p.age_s>=0);
 h+=got.length?chk('ok',`Profil <b>${esc(s.profile_name||s.profile)}</b>: ${got.length} von ${act.length} Werten empfangen (z. B. ${esc(got[0].name)} = ${esc(got[0].value)} ${esc(got[0].unit)}).`)
             :chk('warn',`Profil <b>${esc(s.profile_name||s.profile)}</b>: noch keine Werte – Auto einschalten oder laden lassen.`,'werte');
 if(s.voltage!==undefined)h+=chk(s.voltage<12.2?'warn':'ok',`12-V-Batterie: ${s.voltage.toFixed(1)} V`,s.voltage<12.2?'akku':'');
 if(s.last_error){const e=s.last_error,l=/NO DATA|CAN ERROR/.test(e)?'fehler':/Unvollst|Incomplete/.test(e)?'fehler':/nicht erreichbar/.test(e)?'fehler':'fehler';
  h+=chk('warn','Letzte Meldung: <code>'+esc(e)+'</code>',l)}
 h+=chk('ok','Letzter Neustart: '+esc(s.reset_reason||'–')+' · Laufzeit '+fmt(s.uptime));
 $('checks').innerHTML=h}catch(e){$('checks').innerHTML=chk('err','Status nicht abrufbar.')}}
const fmt=s=>s<60?s+' s':s<3600?Math.round(s/60)+' min':s<86400?Math.round(s/3600)+' h':Math.round(s/86400)+' Tage';
check();setInterval(check,10000);
// ---------- Suche ----------
const secs=[...document.querySelectorAll('main section')].filter(x=>x.id!=='check'&&x.id!=='inhalt');
secs.forEach(x=>x.dataset.html=x.innerHTML);
function search(q){q=q.trim();let any=false;
 secs.forEach(x=>{x.innerHTML=x.dataset.html;
  if(!q){x.classList.remove('hide');return}
  const hit=x.textContent.toLowerCase().includes(q.toLowerCase());x.classList.toggle('hide',!hit);
  if(hit){any=true;x.querySelectorAll('details').forEach(d=>{if(d.textContent.toLowerCase().includes(q.toLowerCase()))d.open=true});mark(x,q)}});
 ['check','inhalt'].forEach(id=>$(id).classList.toggle('hide',!!q));
 $('nores').classList.toggle('hide',!q||any)}
function mark(root,q){const w=document.createTreeWalker(root,NodeFilter.SHOW_TEXT);const nodes=[];while(w.nextNode())nodes.push(w.currentNode);
 const re=new RegExp(q.replace(/[.*+?^${}()|[\]\\]/g,'\\$&'),'gi');
 nodes.forEach(n=>{if(!re.test(n.nodeValue))return;re.lastIndex=0;const s=document.createElement('span');s.innerHTML=esc(n.nodeValue).replace(new RegExp(esc(q).replace(/[.*+?^${}()|[\]\\]/g,'\\$&'),'gi'),m=>'<mark>'+m+'</mark>');n.replaceWith(s)})}
</script>
</body></html>)HTML";

static const char HELP_HTML_EN[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>OBD2MQTT – Manual</title>
<style>
:root{--bg:#f4f5f7;--card:#fff;--fg:#1d2330;--mut:#6b7385;--bd:#dfe3ea;--acc:#1f6feb;--ok:#1a7f37;--err:#cf222e;--warn:#9a6700;--code:#eef1f5}
@media(prefers-color-scheme:dark){:root{--bg:#0f1218;--card:#181c24;--fg:#e6e9ef;--mut:#8b93a5;--bd:#2a303c;--acc:#4c8dff;--ok:#3fb950;--err:#f85149;--warn:#d29922;--code:#11151c}}
*{box-sizing:border-box}body{margin:0;font:16px/1.6 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;background:var(--bg);color:var(--fg)}
header{background:var(--card);border-bottom:1px solid var(--bd);padding:14px 16px;position:sticky;top:0;z-index:2}
header h1{margin:0;font-size:18px}header .sub{color:var(--mut);font-size:13px}
header a{color:var(--acc);text-decoration:none;font-size:14px;float:right;margin-top:4px}
main{max-width:860px;margin:0 auto;padding:16px}
section{background:var(--card);border:1px solid var(--bd);border-radius:12px;padding:6px 20px 14px;margin-bottom:16px}
h2{font-size:19px;margin:16px 0 8px}h3{font-size:16px;margin:16px 0 6px}
a{color:var(--acc)}code,kbd{font-family:ui-monospace,Menlo,Consolas,monospace;font-size:.88em;background:var(--code);border:1px solid var(--bd);border-radius:5px;padding:1px 5px}
table{border-collapse:collapse;width:100%;font-size:14.5px;margin:8px 0}th,td{border-bottom:1px solid var(--bd);padding:6px 6px;text-align:left;vertical-align:top}th{color:var(--mut);font-weight:500}
.toc{columns:2;column-gap:24px;padding-left:18px}@media(max-width:600px){.toc{columns:1}}
.box{border-left:4px solid var(--acc);background:var(--code);padding:8px 12px;border-radius:0 8px 8px 0;margin:10px 0}
.box.warn{border-color:var(--warn)}.box.ok{border-color:var(--ok)}
ol li,ul li{margin:3px 0}
details{border:1px solid var(--bd);border-radius:8px;padding:6px 12px;margin:8px 0}summary{cursor:pointer;font-weight:600}
#q{width:100%;margin-top:10px;font:inherit;padding:8px 11px;border:1px solid var(--bd);border-radius:8px;background:var(--bg);color:var(--fg)}
mark{background:#ffe58a;color:#000;border-radius:3px;padding:0 1px}
.chk{display:flex;gap:10px;align-items:flex-start;padding:7px 0;border-bottom:1px solid var(--bd)}.chk:last-child{border:0}
.dot{flex:none;width:10px;height:10px;border-radius:50%;margin-top:7px;background:var(--mut)}.dot.ok{background:var(--ok)}.dot.err{background:var(--err)}.dot.warn{background:var(--warn)}
.hide{display:none!important}.nores{color:var(--mut);text-align:center;padding:20px}
</style></head><body>
<header><a href="/">← back to the interface</a><a href="/hilfe?lang=de" style="margin-right:14px">🇩🇪 Deutsch</a><h1>OBD2MQTT – Manual</h1><div class="sub" id="ver">User manual</div>
<input id="q" type="search" placeholder="Search the manual … (e.g. “NO DATA”, “update”, “password”)" oninput="search(this.value)"></header>
<main>
<p class="nores hide" id="nores">Nothing found – try another term.</p>

<section id="check">
<h2>Your system right now</h2>
<p style="margin:0 0 6px;color:var(--mut);font-size:14px">Live check of your device – with a link to the matching help.</p>
<div id="checks"><div class="chk"><span class="dot"></span><span>Loading status …</span></div></div>
</section>

<section id="inhalt">
<h2>Contents</h2>
<ol class="toc">
<li><a href="#was">What does the device do?</a></li>
<li><a href="#brauche">What do I need?</a></li>
<li><a href="#install">First installation</a></li>
<li><a href="#einrichten">Setup step by step</a></li>
<li><a href="#bedienung">The interface</a></li>
<li><a href="#ha">Values in Home Assistant</a></li>
<li><a href="#werte">Which values are available?</a></li>
<li><a href="#eigene">Adding your own values</a></li>
<li><a href="#update">Updating the firmware</a></li>
<li><a href="#backup">Backup &amp; restore</a></li>
<li><a href="#akku">12 V battery &amp; saving power</a></li>
<li><a href="#fehler">Troubleshooting</a></li>
<li><a href="#begriffe">Glossary</a></li>
</ol>
</section>

<section id="was">
<h2>1. What does the device do?</h2>
<p>The small ESP32 in your garage connects via <b>Bluetooth</b> to the OBD2 dongle in your car, reads data such as <b>state of charge (SoC)</b>, <b>battery temperature</b> and <b>odometer</b> and sends it via <b>MQTT</b> to <b>Home Assistant</b>.</p>
<p>This is how it fits together:</p>
<div class="box">Car (OBD2 dongle) ⟶ <i>Bluetooth</i> ⟶ ESP32 in the garage ⟶ <i>WiFi</i> ⟶ MQTT broker ⟶ Home Assistant</div>
<p>As soon as the car is within range (typically 5–15 m) and awake – e.g. while charging – the values arrive automatically. If the car is away or asleep, Home Assistant keeps the last value.</p>
</section>

<section id="brauche">
<h2>2. What do I need?</h2>
<ul>
<li>An <b>ESP32</b> (e.g. ESP32-DevKitC) with a USB power supply within Bluetooth range of the car.</li>
<li>A <b>Bluetooth LE OBD2 dongle</b> (ELM327 compatible, “BLE” or “Bluetooth 4.0”), e.g. Vgate iCar Pro BLE, vLinker MC+/FS, Veepeak BLE+, OBDLink CX. <i>Classic Bluetooth dongles (without “LE”) do not work.</i></li>
<li>WiFi in the garage.</li>
<li>An <b>MQTT broker</b> (e.g. the Mosquitto add-on in Home Assistant) with user and password.</li>
<li>Home Assistant with the <b>MQTT integration</b> set up.</li>
</ul>
</section>

<section id="install">
<h2>3. First installation (once, via USB)</h2>
<ol>
<li>Connect the ESP32 to your computer with a USB cable.</li>
<li>Open <a href="https://espressif.github.io/esptool-js/" target="_blank">espressif.github.io/esptool-js</a> in <b>Chrome</b> or <b>Edge</b>.</li>
<li>Click <b>Connect</b> (baud rate 460800) and select the USB port. If it does not work: hold the <kbd>BOOT</kbd> button on the ESP32 while connecting.</li>
<li>Click <b>Erase Flash</b> (removes the old firmware).</li>
<li>Choose the file <code>obd2mqtt-vX.Y.Z-esp32-<b>factory</b>.bin</code>, address <code>0x0</code>, click <b>Program</b>.</li>
<li>After “Done”, briefly disconnect the ESP32 from power and reconnect it.</li>
</ol>
<div class="box warn">The <b>factory.bin</b> is for USB only. For later updates via the interface use the <b>ota.bin</b> (see <a href="#update">chapter 9</a>).</div>
</section>

<section id="einrichten">
<h2>4. Setup step by step</h2>
<h3>4.1 Connect to the device</h3>
<ol>
<li>Connect your phone or laptop to the WiFi <code>OBD2MQTT-xxxx</code> – password <code>obd2mqtt</code>.</li>
<li>Usually the setup page opens by itself, otherwise open <code>http://192.168.4.1</code> in the browser.</li>
<li>Choose your language at the top right (🇩🇪 DE / 🇬🇧 EN).</li>
</ol>
<h3>4.2 WiFi</h3>
<ol>
<li>Tab <b>Settings</b> → next to “SSID” click <b>Scan</b> and select your WiFi.</li>
<li>Enter the WiFi password.</li>
</ol>
<h3>4.3 MQTT</h3>
<p>Enter the broker address (IP of your Home Assistant or MQTT server), port <code>1883</code>, user and password. “Device name in HA” is the name under which the car appears in Home Assistant (e.g. “IONIQ 5”).</p>
<h3>4.4 Pair the dongle</h3>
<ol>
<li>Park the car with the dongle plugged in nearby.</li>
<li>Under <b>BLE dongle</b> click <b>Scan for dongles</b> (takes 6 s).</li>
<li>Click the matching entry – entries marked with ★ are most likely OBD dongles. Leave the UUID fields empty, they are detected automatically. After the first connection the detected dongle (name, type, ELM version) is shown below the button.</li>
</ol>
<h3>4.5 Save</h3>
<p>Click <b>Save &amp; restart</b>. The ESP32 now connects to your WiFi. Afterwards you can reach it at <code>http://obd2mqtt.local</code> or at the IP address assigned by your router (see the device list of your router).</p>
<h3>4.6 Choose the vehicle profile</h3>
<ol>
<li>Tab <b>Profile</b> → choose the matching profile (e.g. “Hyundai IONIQ 5 / Kia EV6” or “XPeng G9”) → <b>Activate</b>.</li>
<li>Switch the car on or let it charge, then click <b>Test</b> next to “soc”. If a plausible value appears, everything is set up.</li>
</ol>
<div class="box ok">Done! From now on the <b>Status</b> tab shows the current values, and a new device appears in Home Assistant.</div>
</section>

<section id="bedienung">
<h2>5. The interface</h2>
<table>
<tr><th>Tab</th><th>What for?</th></tr>
<tr><td><b>Status</b></td><td>Current values, connection status (WiFi, MQTT, Bluetooth), log. <b>Poll now</b> fetches new values immediately. Log buttons: <b>Copy</b>, <b>Download</b>, <b>Delete</b>.</td></tr>
<tr><td><b>Profile</b></td><td>Which values are polled, how often (column “Interval s”) and how they are calculated. <b>Test</b> lets you try any value immediately.</td></tr>
<tr><td><b>Terminal</b></td><td>For advanced users: send commands directly to the dongle.</td></tr>
<tr><td><b>Settings</b></td><td>WiFi, MQTT, dongle, polling times, time of day, password for the interface.</td></tr>
<tr><td><b>System</b></td><td>Version, reason of the last restart, restart, factory profiles, backup &amp; restore, changelog, firmware update.</td></tr>
<tr><td><b>🇩🇪/🇬🇧</b></td><td>Language of the interface, the device messages and the Home Assistant names. The choice is stored on the device.</td></tr>
</table>
<h3>Important settings</h3>
<table>
<tr><th>Setting</th><th>Meaning</th><th>Default</th></tr>
<tr><td>Polling ON/OFF</td><td>Main switch on the status page (and in Home Assistant as switch “Polling”). When OFF the bridge no longer connects to the car – e.g. at the workshop or during long parking. Test, terminal and “Poll now” keep working. Survives a restart.</td><td>ON</td></tr>
<tr><td>12V minimum voltage</td><td>Below this value no polling takes place, to protect the 12 V battery.</td><td>12.2 V</td></tr>
<tr><td>Car asleep below</td><td>If the 12 V voltage is below this value, the car is off and not charging. The bridge then sends <b>no requests to the car</b> (each one would wake it up) and only measures the voltage at the dongle once a minute. As soon as the car wakes up by itself (driving, charging, preconditioning, 12 V top-up), normal polling resumes. An awake IONIQ 5 is at about 14.7 V, a sleeping one at 12.4–12.8 V. With a 12 V lithium battery (resting voltage ~13.3 V) raise the value to about 13.6 V. 0 = always poll.</td><td>13.2 V</td></tr>
<tr><td>Poll while car is asleep</td><td>Poll once every N minutes even while asleep (briefly wakes the car). 0 = never. “Poll now” always polls.</td><td>0</td></tr>
<tr><td>Reconnect attempt after</td><td>Waiting time if the dongle is not reachable (car away).</td><td>60 s</td></tr>
<tr><td>Pause when car does not respond</td><td>Waiting time if the car is asleep.</td><td>600 s</td></tr>
<tr><td>Keep BLE connected permanently</td><td>“no” = dongle may sleep between polls (recommended).</td><td>no</td></tr>
<tr><td>IP address</td><td>“automatic (DHCP)” or a static address. “Use current values” fills in IP, mask, gateway and DNS of the running connection. The address must be outside the router's DHCP range. If the static IP does not work after the restart (no WiFi or MQTT broker not reachable), the bridge automatically uses DHCP and reports this in the log and under System.</td><td>DHCP</td></tr>
<tr><td>Web password</td><td>Protects the interface. Log in with user <code>admin</code>.</td><td>empty</td></tr>
<tr><td>Time zone</td><td>For the time in the log. Choose from the list; the current time on the device is shown below.</td><td>Central Europe</td></tr>
<tr><td>Store log permanently</td><td>Log survives restarts – helpful for troubleshooting.</td><td>yes</td></tr>
</table>
</section>

<section id="ha">
<h2>6. Values in Home Assistant</h2>
<p>The values appear <b>automatically</b> – you do not need to configure anything in Home Assistant. Under <i>Settings → Devices &amp; services → MQTT</i> you will find a device with the configured name and, among others, these sensors:</p>
<table>
<tr><th>Sensor</th><th>Meaning</th></tr>
<tr><td>SoC</td><td>State of charge of the traction battery in %</td></tr>
<tr><td>Battery temp max / min</td><td>Highest/lowest temperature in the battery</td></tr>
<tr><td>Odometer</td><td>Total km</td></tr>
<tr><td>12V battery</td><td>Voltage of the 12 V battery</td></tr>
<tr><td>OBD dongle connected</td><td>“Connected” if the car was reachable last time</td></tr>
<tr><td>Vehicle profile</td><td>Name of the active profile (details as attributes)</td></tr>
<tr><td>BLE signal / Last error</td><td>Diagnostics</td></tr>
</table>
<p>You can use the SoC e.g. in <b>evcc</b> as vehicle SoC or for automations (“notify me at 80 %”).</p>
<p>For your own evaluations the device additionally sends all values including the profile as one JSON to <code>obd2mqtt/state</code> after each poll, e.g. <code>{"profile":"XPeng G9","values":{"soc":71.5,…}}</code>.</p>
</section>

<section id="werte">
<h2>7. Which values are available?</h2>
<p>In the <b>Profile</b> tab you can enable or disable individual values with the checkbox in column “Active” – the checkbox in the column header switches all at once – and then click <b>Save profile</b>. In the factory profiles all values are active.</p>
<h3>Hyundai IONIQ 5 (also Kia EV6, IONIQ 6)</h3>
<p>All values are active: SoC, battery temperature max/min, odometer, SoH (battery health), raw BMS SoC and calculated range.</p>
<h3>XPeng G9, G6 and P7+</h3>
<p>One profile each (<code>g9</code>, <code>g6</code>, <code>p7plus</code>) with the same values, all active: SoC, battery temperature max/min, odometer, range (CLTC), SoH, charge limit, charging status, HV voltage/current, calculated range and the standard OBD2 SoC.</p>
<p>The <b>G6</b> is verified best. According to user reports the G9 uses the same commands. For the <b>P7+</b> no values have been published yet – the profile is an attempt based on G6/G9.</p>
<div class="box warn">The XPeng values come from the community and have not been verified for every model year – compare them once with the display in the car.</div>
<h3>Range</h3>
<p>The range shown in the cockpit cannot be read. “Range (calculated)” estimates it from SoC × battery capacity ÷ consumption. Adapt the formula to your car, e.g. <code>B34/2/100*63/0.17</code> for 63 kWh and 17 kWh/100 km. The XPeng additionally offers the “CLTC range” – usually 15–20 % more optimistic than WLTP.</p>
<h3>How often is polled?</h3>
<p>Column <b>Interval s</b> in the profile: SoC every 120 s, temperatures every 300 s, odometer every 1800 s. Values with the same command are best polled with the same interval.</p>
<p>Note: the value names of the factory profiles follow the language that is active when they are installed. After switching the language, <b>Restore factory profiles</b> (System tab) renames them.</p>
</section>

<section id="eigene">
<h2>8. Adding your own values (for the curious)</h2>
<ol>
<li>Profile → <b>+ PID</b>.</li>
<li>Enter <b>Header</b> (control unit, e.g. <code>7E4</code>) and <b>Command</b> (e.g. <code>220101</code>) – you can find such information in forums or projects such as evDash, WiCAN or OVMS.</li>
<li>First enter <code>B3</code> as formula and press <b>Test</b>. You will see all received bytes with their number (B0, B1, …).</li>
<li>Adjust the formula until the value is correct. Helpers: <code>u16(B3,B4)</code> = 2-byte number, <code>s8(B5)</code> = signed number, <code>bit(B6,2)</code> = single bit.</li>
<li>Set unit, class and interval, tick “Active” and click <b>Save profile</b>. Home Assistant creates the sensor automatically.</li>
</ol>
<p>Tip: use <b>Save copy…</b> beforehand to back up your profile – just enter a name, e.g. “My G9”. You change the display name of a profile in the <b>Name</b> field.</p>
</section>

<section id="update">
<h2>9. Updating the firmware</h2>
<ol>
<li>Tab <b>System</b> → under “Firmware update” choose the file <code>obd2mqtt-vX.Y.Z-esp32-<b>ota</b>.bin</code>.</li>
<li>Click <b>Upload</b>. The bar shows the progress, afterwards the ESP32 restarts and the page shows the new version.</li>
</ol>
<p>Settings and profiles are kept. What has changed is listed in the <b>Changelog</b> in the System tab. New factory profiles are applied with <b>Restore factory profiles</b> (overwrites your own changes to <code>ioniq5</code>/<code>g9</code>).</p>
</section>

<section id="backup">
<h2>9a. Backup &amp; restore</h2>
<p>In the <b>System</b> tab → “Backup &amp; restore”:</p>
<ul>
<li><b>Download backup</b> saves all settings and vehicle profiles as file <code>obd2mqtt-backup-DATE.json</code>.</li>
<li>Checkbox <b>incl. passwords</b>: only tick it if you store the file securely – it then contains the WiFi, MQTT and web password in plain text.</li>
<li><b>Restore</b>: choose the file, select whether <b>Settings</b> and/or <b>Profiles</b> should be applied, confirm. The device restarts afterwards.</li>
<li>With a backup without passwords, the currently stored passwords are kept.</li>
</ul>
<div class="box">Tip: before re-flashing with “Erase Flash”, always take a backup including passwords – afterwards the device is set up again within a minute (connect to the setup WiFi → System → Restore).</div>
</section>

<section id="akku">
<h2>10. 12 V battery &amp; saving power</h2>
<ul>
<li>The dongle stays plugged into the car and needs a little power. Use a dongle with <b>automatic sleep mode</b>.</li>
<li>The firmware disconnects Bluetooth between polls so that the dongle can sleep.</li>
<li>If the car does not respond (it is asleep), polling pauses for 10 minutes.</li>
<li>If the 12 V voltage drops below the minimum voltage, polling is paused.</li>
<li>If you do not drive for a longer time (holiday): unplug the dongle.</li>
</ul>
</section>

<section id="fehler">
<h2>11. Troubleshooting</h2>
<details><summary>“NO DATA” or “CAN ERROR”</summary>
<p>The car is asleep. The control units only respond when the car is switched on or charging. This is normal – Home Assistant keeps the last value. For testing: switch the car on or plug in the charging cable.</p></details>
<details><summary>“Dongle not reachable”</summary>
<p>Car not within range, dongle asleep or the Bluetooth connection is too weak. “BLE signal” should be better than about −85 dBm – otherwise move the ESP32 closer to the car. Important: the dongle must not be connected to a phone app at the same time.</p></details>
<details><summary>No values arrive in Home Assistant</summary>
<p>Check in the Status tab whether “MQTT” is green. If not: check broker address, user and password. The MQTT integration must be set up in Home Assistant.</p></details>
<details><summary>“Incomplete response” in the log</summary>
<p>The dongle did not deliver a long response completely. The firmware then retries automatically. If it happens often: add <code>ATST96</code> to the init commands in the profile.</p></details>
<details><summary>A value is obviously wrong</summary>
<p>Click <b>Test</b> next to the value in the profile and compare with the display in the car. Often just the byte number in the formula is wrong (see <a href="#eigene">chapter 8</a>).</p></details>
<details><summary>Interface not reachable</summary>
<p>Briefly disconnect the ESP32 from power. If the WiFi is not reachable for 3 minutes, it opens the setup WiFi <code>OBD2MQTT-xxxx</code> again. If the firmware hangs for longer than 60 s, it restarts by itself.</p></details>
<details><summary>Red notice “Safe mode”</summary>
<p>The firmware crashed several times in a row and therefore runs without Bluetooth. Check recently changed settings or the profile and then click <b>Restart normally</b>. The stored log shows what happened before.</p></details>
<details><summary>I forgot the web password</summary>
<p>Reinstall via USB with <b>Erase Flash</b> (<a href="#install">chapter 3</a>). Afterwards all settings are deleted – with a <a href="#backup">backup</a> (without passwords) everything else is back quickly.</p></details>
<details><summary>Asking for help</summary>
<p>In the Status tab click <b>Copy</b> at the log and pass on the text together with the version (top left).</p></details>
</section>

<section id="begriffe">
<h2>12. Glossary</h2>
<table>
<tr><td><b>OBD2</b></td><td>Diagnostic connector in the car, usually in the footwell on the driver's side.</td></tr>
<tr><td><b>Dongle / ELM327</b></td><td>The small adapter in the OBD2 connector that provides the data via Bluetooth.</td></tr>
<tr><td><b>BLE</b></td><td>Bluetooth Low Energy – power-saving Bluetooth.</td></tr>
<tr><td><b>SoC / SoH</b></td><td>State of Charge / State of Health of the battery.</td></tr>
<tr><td><b>PID</b></td><td>A value that can be queried, or the command for it.</td></tr>
<tr><td><b>Header</b></td><td>Address of the control unit in the car that is queried (e.g. 7E4 = battery management on the IONIQ 5).</td></tr>
<tr><td><b>MQTT</b></td><td>Messaging protocol through which Home Assistant receives the values.</td></tr>
<tr><td><b>OTA</b></td><td>“Over the air” – firmware update via WiFi.</td></tr>
<tr><td><b>RSSI</b></td><td>Signal strength in dBm. −60 is very good, −90 very weak.</td></tr>
</table>
</section>
</main>

<script>
const $=id=>document.getElementById(id);
const esc=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
// ---------- Live-Prüfung ----------
function chk(state,text,link){return `<div class="chk"><span class="dot ${state}"></span><span>${text}${link?` – <a href="#${link}" onclick="openSec('${link}')">Help</a>`:''}</span></div>`}
function openSec(id){search('');$('q').value='';const d=document.querySelector('#'+id+' details');if(d)d.open=true}
async function check(){try{const s=await (await fetch('/api/status')).json();
 $('ver').textContent='User manual · firmware v'+s.fw;
 let h='';
 h+=s.safe_mode?chk('err','<b>Safe mode active</b> – Bluetooth is off.','fehler'):'';
 h+=s.wifi.connected?chk('ok',`WiFi connected to <b>${esc(s.wifi.ssid)}</b> (${s.wifi.rssi} dBm)`):chk('err','No WiFi – check the setup.','einrichten');
 h+=s.mqtt?chk('ok','MQTT connected – values go to Home Assistant.'):chk('err','MQTT not connected – no values in Home Assistant.','fehler');
 if(!s.ble.mac)h+=chk('warn','No dongle set up yet.','einrichten');
 else if(s.ble.connected)h+=chk('ok',`Dongle connected (${s.ble.rssi} dBm)`);
 else h+=chk(s.ble.last_ok_s>=0?'ok':'warn',`Dongle ${esc(s.ble.mac)} currently not connected${s.ble.last_ok_s>=0?' – last reachable '+fmt(s.ble.last_ok_s)+' ago (normal: Bluetooth is disconnected between polls)':' – never reached'}.`,s.ble.last_ok_s>=0?'':'fehler');
 const act=(s.pids||[]).filter(p=>p.enabled);
 const got=act.filter(p=>p.age_s>=0);
 h+=got.length?chk('ok',`Profile <b>${esc(s.profile_name||s.profile)}</b>: ${got.length} of ${act.length} values received (e.g. ${esc(got[0].name)} = ${esc(got[0].value)} ${esc(got[0].unit)}).`)
             :chk('warn',`Profile <b>${esc(s.profile_name||s.profile)}</b>: no values yet – switch the car on or let it charge.`,'werte');
 if(s.voltage!==undefined)h+=chk(s.voltage<12.2?'warn':'ok',`12 V battery: ${s.voltage.toFixed(1)} V`,s.voltage<12.2?'akku':'');
 if(s.last_error){const e=s.last_error,l=/NO DATA|CAN ERROR/.test(e)?'fehler':/Unvollst|Incomplete/.test(e)?'fehler':/nicht erreichbar/.test(e)?'fehler':'fehler';
  h+=chk('warn','Last message: <code>'+esc(e)+'</code>',l)}
 h+=chk('ok','Last restart: '+esc(s.reset_reason||'–')+' · uptime '+fmt(s.uptime));
 $('checks').innerHTML=h}catch(e){$('checks').innerHTML=chk('err','Status not available.')}}
const fmt=s=>s<60?s+' s':s<3600?Math.round(s/60)+' min':s<86400?Math.round(s/3600)+' h':Math.round(s/86400)+' days';
check();setInterval(check,10000);
// ---------- Suche ----------
const secs=[...document.querySelectorAll('main section')].filter(x=>x.id!=='check'&&x.id!=='inhalt');
secs.forEach(x=>x.dataset.html=x.innerHTML);
function search(q){q=q.trim();let any=false;
 secs.forEach(x=>{x.innerHTML=x.dataset.html;
  if(!q){x.classList.remove('hide');return}
  const hit=x.textContent.toLowerCase().includes(q.toLowerCase());x.classList.toggle('hide',!hit);
  if(hit){any=true;x.querySelectorAll('details').forEach(d=>{if(d.textContent.toLowerCase().includes(q.toLowerCase()))d.open=true});mark(x,q)}});
 ['check','inhalt'].forEach(id=>$(id).classList.toggle('hide',!!q));
 $('nores').classList.toggle('hide',!q||any)}
function mark(root,q){const w=document.createTreeWalker(root,NodeFilter.SHOW_TEXT);const nodes=[];while(w.nextNode())nodes.push(w.currentNode);
 const re=new RegExp(q.replace(/[.*+?^${}()|[\]\\]/g,'\\$&'),'gi');
 nodes.forEach(n=>{if(!re.test(n.nodeValue))return;re.lastIndex=0;const s=document.createElement('span');s.innerHTML=esc(n.nodeValue).replace(new RegExp(esc(q).replace(/[.*+?^${}()|[\]\\]/g,'\\$&'),'gi'),m=>'<mark>'+m+'</mark>');n.replaceWith(s)})}
</script>
</body></html>)HTML";
