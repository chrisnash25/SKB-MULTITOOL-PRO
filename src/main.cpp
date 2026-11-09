#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <M5Unified.h>
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <IRremote.hpp>
#include <LittleFS.h>

static WebServer server(80);
static constexpr uint8_t IR_PIN = 19;
static bool filesystemReady = false;
static File logoUploadFile;
static Preferences skbPrefs;
static BLEScan* bleScanner = nullptr;
static String trackedBleAddress;
static bool trackedBleSeen = false;
static int trackedBleRssi = -127;
static unsigned long trackedBleLastSeenMs = 0;
static bool pathShieldEnabled = false;
static unsigned int trackerCandidatesLastScan = 0;
static String trackerAlert = "No BLE scan yet";
static unsigned long pathShieldEnabledSince = 0;

void sendPathShieldStatus() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["enabled"] = pathShieldEnabled;
  doc["scanner_ready"] = (bleScanner != nullptr);
  doc["mode"] = "ble_candidate_classification";
  doc["candidate_count"] = trackerCandidatesLastScan;
  doc["alert"] = trackerAlert;
  doc["note"] = "Candidate counts are informational, not proof of unwanted tracking.";
  String json; serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void setPathShieldEnabled() {
  if (!server.hasArg("plain")) {server.send(400,"application/json","{\"ok\":false,\"error\":\"missing_body\"}");return;}
  JsonDocument doc;
  if (deserializeJson(doc,server.arg("plain")) || !doc["enabled"].is<bool>()) {
    server.send(400,"application/json","{\"ok\":false,\"error\":\"expected_boolean_enabled\"}");return;
  }
  pathShieldEnabled=doc["enabled"].as<bool>();
  skbPrefs.putBool("pathshield",pathShieldEnabled);
  pathShieldEnabledSince=millis();
  sendPathShieldStatus();
}

static const char* apName = "SKB-MULTITOOL-PRO";
static String input;
static bool wifiScanning = false;
static unsigned long scanStarted = 0;
static String cachedWifiJson = "{\"ok\":true,\"state\":\"idle\",\"networks\":[]}";
static String cachedSignalJson = "{\"ok\":true,\"source\":\"last_wifi_scan\",\"live\":false,\"signals\":[]}";

void startWifiScan() {
  if (wifiScanning) { server.send(409, "application/json", "{\"ok\":false,\"error\":\"scan_running\"}"); return; }
  WiFi.scanDelete();
  WiFi.scanNetworks(true, true);
  wifiScanning = true;
  scanStarted = millis();
  server.send(202, "application/json", "{\"ok\":true,\"state\":\"scanning\"}");
}

void wifiResults() {
  JsonDocument doc;
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) {
    if (millis() - scanStarted > 15000) { WiFi.scanDelete(); wifiScanning = false; server.send(504, "application/json", "{\"ok\":false,\"error\":\"scan_timeout\"}"); return; }
    doc["state"] = "scanning";
  } else if (n < 0) {
    wifiScanning = false;
    doc["state"] = "idle";
  } else {
    wifiScanning = false;
    doc["state"] = "complete";
    JsonArray aps = doc["networks"].to<JsonArray>();
    for (int i=0; i<n && i<64; ++i) {
      JsonObject ap = aps.add<JsonObject>();
      ap["ssid"] = WiFi.SSID(i);
      ap["bssid"] = WiFi.BSSIDstr(i);
      ap["rssi"] = WiFi.RSSI(i);
      ap["channel"] = WiFi.channel(i);
      ap["encryption"] = static_cast<int>(WiFi.encryptionType(i));
    }
    JsonDocument signalDoc;
    signalDoc["ok"] = true;
    signalDoc["source"] = "last_wifi_scan";
    signalDoc["live"] = false;
    JsonArray signalArray = signalDoc["signals"].to<JsonArray>();
    for (int i=0; i<n && i<64; ++i) {
      JsonObject entry = signalArray.add<JsonObject>();
      entry["ssid"] = WiFi.SSID(i);
      entry["bssid"] = WiFi.BSSIDstr(i);
      entry["rssi"] = WiFi.RSSI(i);
      entry["channel"] = WiFi.channel(i);
    }
    cachedSignalJson = "";
    serializeJson(signalDoc,cachedSignalJson);
    WiFi.scanDelete();
  }
  doc["ok"] = true;
  String json; serializeJson(doc,json);
  if (n >= 0) cachedWifiJson = json;
  server.send(200,"application/json",json);
}


void scanBle() {
  if (!bleScanner) {server.send(503,"application/json","{\"ok\":false,\"error\":\"ble_not_ready\"}");return;}
  BLEScanResults results = bleScanner->start(3, false);
  JsonDocument doc;
  doc["ok"] = true;
  doc["source"] = "ble_advertisements";
  doc["duration_seconds"] = 3;
  JsonArray devices = doc["devices"].to<JsonArray>();
  bool foundTracked = false;
  unsigned int candidateCount = 0;
  int count = results.getCount();
  for (int i=0; i<count && i<40; ++i) {
    BLEAdvertisedDevice device = results.getDevice(i);
    JsonObject item = devices.add<JsonObject>();
    String address = String(device.getAddress().toString().c_str());
    item["address"] = address;
    if (trackedBleAddress.length() && address.equalsIgnoreCase(trackedBleAddress)) {
      foundTracked = true;
      trackedBleRssi = device.getRSSI();
      trackedBleLastSeenMs = millis();
    }
    item["rssi"] = device.getRSSI();
    if (device.haveName()) item["name"] = String(device.getName().c_str());
    if (device.haveManufacturerData()) item["manufacturer_data_bytes"] = device.getManufacturerData().length();
    if (device.haveServiceUUID()) item["service_uuid"] = String(device.getServiceUUID().toString().c_str());
    if (pathShieldEnabled) {
      String candidate = "";
      if (device.haveManufacturerData()) {
        std::string md = device.getManufacturerData();
        if (md.length() >= 3) {
          uint16_t company = (uint8_t)md[0] | ((uint16_t)(uint8_t)md[1] << 8);
          if (company == 0x004C && md.length() >= 27 && ((uint8_t)md[2] == 0x12 || (uint8_t)md[2] == 0x07)) candidate = "Find My-compatible candidate";
          else if (company == 0x0075) candidate = "Samsung manufacturer (tracker unconfirmed)";
          else if (company == 0x0133) candidate = "Chipolo manufacturer candidate";
        }
      }
      if (device.haveServiceUUID()) {
        String uuid = String(device.getServiceUUID().toString().c_str());
        uuid.toLowerCase();
        if (uuid.indexOf("fd51") >= 0 || uuid.indexOf("fd52") >= 0) candidate = "Tile-compatible service candidate";
        else if (uuid.indexOf("fd6f") >= 0) candidate = "SmartThings-compatible service candidate";
      }
      if (candidate.length()) { item["tracker_candidate"] = candidate; ++candidateCount; }
    }
  }
  trackedBleSeen = foundTracked;
  if (pathShieldEnabled) {
    trackerCandidatesLastScan = candidateCount;
    trackerAlert = candidateCount ? String(candidateCount) + " tracker-related advertisement candidate(s)" : "No tracker candidates found in last scan";
  }
  JsonObject tracking = doc["tracking"].to<JsonObject>();
  tracking["address"] = trackedBleAddress;
  tracking["seen_this_scan"] = foundTracked;
  if (foundTracked) tracking["rssi"] = trackedBleRssi;
  tracking["last_seen_uptime_ms"] = trackedBleLastSeenMs;
  tracking["note"] = "Not seen does not guarantee departure; BLE addresses may rotate.";
  doc["pathshield_candidates"] = trackerCandidatesLastScan;
  doc["pathshield_alert"] = trackerAlert;
  String json;
  serializeJson(doc,json);
  bleScanner->clearResults();
  server.send(200,"application/json",json);
}

void bleTrackingSettings() {
  if (server.method() == HTTP_POST) {
    JsonDocument request;
    if (deserializeJson(request,server.arg("plain")) || !request["address"].is<const char*>()) {
      server.send(400,"application/json","{\"ok\":false,\"error\":\"address_required\"}"); return;
    }
    String address = request["address"].as<String>();
    address.trim();
    if (address.length() && (address.length()!=17 || address.charAt(2)!=':' || address.charAt(5)!=':' || address.charAt(8)!=':' || address.charAt(11)!=':' || address.charAt(14)!=':')) {
      server.send(400,"application/json","{\"ok\":false,\"error\":\"invalid_ble_address\"}"); return;
    }
    trackedBleAddress = address;
    trackedBleSeen = false;
    trackedBleRssi = -127;
    trackedBleLastSeenMs = 0;
    skbPrefs.putString("ble_target",trackedBleAddress);
  }
  JsonDocument response;
  response["ok"] = true;
  response["address"] = trackedBleAddress;
  response["seen_this_scan"] = trackedBleSeen;
  if (trackedBleSeen) response["rssi"] = trackedBleRssi;
  response["last_seen_uptime_ms"] = trackedBleLastSeenMs;
  String json; serializeJson(response,json);
  server.send(200,"application/json",json);
}

void sendStatus() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["device"] = "SKB MULTITOOL PRO";
  doc["uptime_ms"] = millis();
  doc["free_heap"] = ESP.getFreeHeap();
  doc["ap_ip"] = WiFi.softAPIP().toString();
  doc["battery_voltage"] = M5.Power.getBatteryVoltage();
  doc["pathshield_enabled"] = pathShieldEnabled;
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void onSerialLine(const String& line) {
  JsonDocument request;
  JsonDocument response;
  DeserializationError error = deserializeJson(request, line);
  if (error) {
    Serial.println("{\"ok\":false,\"error\":\"invalid_json\"}");
    return;
  }
  String cmd = request["cmd"] | "";
  if (cmd == "status") {
    response["ok"] = true;
    response["device"] = "SKB MULTITOOL PRO";
    response["uptime_ms"] = millis();
    response["free_heap"] = ESP.getFreeHeap();
    response["battery_voltage"] = M5.Power.getBatteryVoltage();
  } else {
    response["ok"] = false;
    response["error"] = "unknown_command";
  }
  serializeJson(response, Serial);
  Serial.println();
}

void sendIR() {
  if (!server.hasArg("plain")) { server.send(400,"application/json","{\"ok\":false,\"error\":\"missing_body\"}"); return; }
  JsonDocument request;
  if (deserializeJson(request,server.arg("plain")) || !request["address"].is<int>() || !request["command"].is<int>()) {
    server.send(400,"application/json","{\"ok\":false,\"error\":\"address_and_command_required\"}"); return;
  }
  int address = request["address"].as<int>(), command = request["command"].as<int>();
  if (address<0 || address>65535 || command<0 || command>255) {
    server.send(400,"application/json","{\"ok\":false,\"error\":\"invalid_nec_values\"}"); return;
  }
  IrSender.sendNEC((uint16_t)address,(uint8_t)command,0);
  server.send(200,"application/json","{\"ok\":true,\"protocol\":\"NEC\",\"repeats\":0}");
}
void uploadLogo() {
  HTTPUpload& upload=server.upload();
  if (!filesystemReady) return;
  if (upload.status==UPLOAD_FILE_START) {
    if (logoUploadFile) logoUploadFile.close();
    if (upload.filename.endsWith(".png") || upload.filename.endsWith(".PNG")) {
      logoUploadFile=LittleFS.open("/logo.png","w");
    }
  } else if (upload.status==UPLOAD_FILE_WRITE) {
    if (logoUploadFile && upload.totalSize<=150000 && upload.currentSize<=150000-upload.totalSize) logoUploadFile.write(upload.buf,upload.currentSize);
  } else if (upload.status==UPLOAD_FILE_END || upload.status==UPLOAD_FILE_ABORTED) {
    if (logoUploadFile) logoUploadFile.close();
    if (upload.status==UPLOAD_FILE_ABORTED || upload.totalSize>150000) LittleFS.remove("/logo.png");
  }
}
void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  M5.begin(cfg);
  skbPrefs.begin("skb-settings",false);
  filesystemReady=LittleFS.begin(true);
  IrSender.begin(IR_PIN);
  pathShieldEnabled=skbPrefs.getBool("pathshield",false);
  trackedBleAddress=skbPrefs.getString("ble_target","");
  // Headless-first: no screen interaction required.
  M5.Display.setRotation(0);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(8, 20);
  M5.Display.println("SKB MULTITOOL");
  M5.Display.setTextSize(1);
  M5.Display.println("  PRO");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apName);
  BLEDevice::init("SKB-MULTITOOL-PRO");
  bleScanner = BLEDevice::getScan();
  bleScanner->setActiveScan(false);
  server.on("/api/status", HTTP_GET, sendStatus);
  server.on("/api/ir/nec", HTTP_POST, sendIR);
  server.on("/api/logo", HTTP_POST, [](){server.send(filesystemReady?200:503,"application/json",filesystemReady?"{\"ok\":true}":"{\"ok\":false}");}, uploadLogo);
  server.on("/api/ble/scan", HTTP_GET, scanBle);
  server.on("/api/ble/tracking", HTTP_GET, bleTrackingSettings);
  server.on("/api/ble/tracking", HTTP_POST, bleTrackingSettings);
  server.on("/api/pathshield", HTTP_GET, sendPathShieldStatus);
  server.on("/api/pathshield", HTTP_POST, setPathShieldEnabled);
  server.on("/api/wifi/scan", HTTP_POST, startWifiScan);
  server.on("/api/wifi/results", HTTP_GET, wifiResults);
  server.on("/api/signal/wifi", HTTP_GET, []() { server.send(200,"application/json",cachedSignalJson); });
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", R"SKBHTML(
<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>SKB MULTITOOL PRO</title>
<style>
:root{color-scheme:dark;--ink:#080c12;--panel:#121a24;--line:#30475a;--blue:#50d4ff;--silver:#d7e0e9}
*{box-sizing:border-box}html{scroll-behavior:smooth}body{margin:0;color:#edf4fa;background:radial-gradient(ellipse at 75% -12%,#173e59 0%,transparent 48%),linear-gradient(180deg,#080d13,#06080d);font:14px/1.55 system-ui,-apple-system,Arial}
main{max-width:620px;margin:auto;padding:16px 15px 40px}.hero,.panel{position:relative;overflow:hidden;background:linear-gradient(145deg,rgba(34,47,63,.94),rgba(11,17,25,.98));border:1px solid var(--line);border-radius:18px;padding:18px;margin:0 0 14px;box-shadow:0 12px 28px #0005,inset 0 1px #ffffff12}
.hero{padding:28px 18px;text-align:center;border-color:#4b647a;background:radial-gradient(circle at 50% 0%,#28465b 0%,#111924 62%,#0b111a 100%)}
.hero:before{content:"";position:absolute;inset:0 0 auto;height:2px;background:linear-gradient(90deg,transparent,#7fe5ff,transparent)}
.hero img{width:130px;height:130px;object-fit:contain;filter:drop-shadow(0 9px 14px #0009);border-radius:15px}
h1{font-size:24px;font-weight:900;letter-spacing:1.3px;margin:13px 0 3px;color:#f3f7fc}h2{font-size:17px;letter-spacing:.2px;margin:4px 0 10px}p{margin:8px 0 13px}
small,.muted{color:#a0b5c8}.tag{color:var(--blue);font-size:10px;letter-spacing:2px;font-weight:800;text-transform:uppercase}
button{appearance:none;background:linear-gradient(180deg,#205570,#123346);border:1px solid #49bce7;border-radius:11px;color:#effbff;font-weight:800;padding:12px 15px;cursor:pointer;min-height:44px;box-shadow:inset 0 1px #ffffff1e,0 3px 12px #0004}
button:active{transform:translateY(1px);filter:brightness(1.2)}button:focus-visible,input:focus-visible{outline:2px solid #a4eeff;outline-offset:2px}
input[type=number],input[type=text],input:not([type]){width:100%;padding:12px;margin:5px 0 10px;background:#08131f;color:#f3f8ff;border:1px solid #3d6078;border-radius:10px;font:inherit}
input[type=file]{display:block;width:100%;margin:10px 0;color:#b8cddd;font-size:12px}input[type=checkbox]{accent-color:#56cfff;width:18px;height:18px;vertical-align:middle}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}.grid .panel{margin:0 0 14px}.grid button{width:100%}
pre{white-space:pre-wrap;overflow-wrap:anywhere;font:12px/1.6 ui-monospace,SFMono-Regular,Consolas,monospace;color:#a9e5fa;background:#070e17;border:1px solid #243a4c;border-radius:11px;padding:12px;max-height:320px;overflow:auto}
a{color:#7bdcff}.nav{display:flex;gap:8px;overflow:auto;padding:0 0 14px}.nav a{flex:none;text-decoration:none;border:1px solid #38576e;background:#12212e;border-radius:99px;color:#bfe8fa;padding:8px 12px;font-size:12px;font-weight:800}
.status-dot{display:inline-block;width:7px;height:7px;background:#52dba5;border-radius:50%;box-shadow:0 0 12px #52dba5;margin-right:5px}
@media(max-width:360px){main{padding:10px}.grid{grid-template-columns:1fr}.hero img{width:110px;height:110px}}
</style><main>
<header class="hero"><img id="skb-emblem" src="data:image/jpeg;base64,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" alt="Original stitched SKB emblem">
<h1>SKB MULTITOOL PRO</h1><div><label class="muted">Customise emblem · stitched SKB PNG (max 150 KB) <input id="logo-file" type="file" accept="image/png"></label><button onclick="uploadBrandLogo()">Apply logo</button><small id="logo-result"></small></div><div class="tag">HEADLESS CONTROL CONSOLE</div><p class="muted">M5StickC Plus2 / Mobile command centre</p></header>
<nav class="nav"><a href="#device">System</a><a href="#infrared">Infrared</a><a href="#bluetooth">Bluetooth</a><a href="#privacy">PathShield</a><a href="#wifi">Wi-Fi</a></nav><section id="device" class="panel"><div class="tag"><span class="status-dot"></span>DEVICE / STATUS</div><h2>System status</h2><pre id="status">Connecting...</pre><button onclick="statusNow()">Refresh</button></section>
<section id="infrared" class="panel"><div class="tag">INFRARED / TX</div><h2>NEC IR Remote</h2><p class="muted">Send a single NEC command to a device you own. Enter the address and command from its remote specification (decimal values).</p><input id="ir-address" type="number" min="0" max="65535" placeholder="Address (0–65535)"><input id="ir-command" type="number" min="0" max="255" placeholder="Command (0–255)"><button onclick="sendIr()">Transmit</button><pre id="ir-result">Ready</pre></section>
<div class="grid"><section class="panel"><div class="tag">WI-FI</div><h2>Network scan</h2><button onclick="scan()">Scan</button></section>
<section class="panel"><div class="tag">BLUETOOTH</div><h2>BLE discovery</h2><p class="muted">BLE advertisement discovery</p><button onclick="scanBle()">Discover</button></section></div>
<section id="bluetooth" class="panel"><div class="tag">BLUETOOTH RESULTS</div><pre id="ble-results">Tap Discover to scan BLE advertisements.</pre><h2>Track one of your devices</h2><input id="ble-target" placeholder="BLE address AA:BB:CC:DD:EE:FF" style="width:100%;padding:12px;background:#0b1723;color:#fff;border:1px solid #34bfff;border-radius:10px;margin:8px 0"><button onclick="saveBleTarget()">Save target</button> <button onclick="clearBleTarget()">Clear</button><pre id="ble-tracking">No target configured</pre></section>
<section id="privacy" class="panel"><div class="tag">SETTINGS / PRIVACY</div><h2>PathShield</h2><p class="muted">BLE tracker-candidate classification and last-scan counts. These are not proof of unwanted tracking.</p><label><input type="checkbox" id="pathshield" onchange="setPathShield(this.checked)"> Enable PathShield mode</label><pre id="pathshield-state">Loading...</pre></section>
<section class="panel"><div class="tag">SIGNAL FINDER</div><h2>Wi-Fi strength</h2><p class="muted">Signal levels from completed Wi-Fi scans; refresh by scanning again. Live tracking and BLE proximity alerts are not yet available.</p><button onclick="signalWifi()">View signal levels</button><pre id="signal-wifi">No readings yet</pre></section>
<section id="wifi" class="panel"><h2>Nearby Wi-Fi</h2><pre id="networks">Tap Scan to discover networks.</pre></section>
<p class="muted">Access point: SKB-MULTITOOL-PRO • http://192.168.4.1</p>
</main><script>
async function scanBle(){const el=document.getElementById('ble-results');el.textContent='Scanning for 3 seconds...';try{const r=await fetch('/api/ble/scan');const d=await r.json();if(!r.ok)throw Error(JSON.stringify(d));el.textContent=(d.devices||[]).map(n=>(n.name||'Unnamed')+'  '+n.address+'  '+n.rssi+' dBm'+(n.tracker_candidate?' ['+n.tracker_candidate+']':'')).join('\\n')||'No BLE advertisements found';document.getElementById('ble-tracking').textContent=JSON.stringify(d.tracking,null,2);pathShieldStatus()}catch(e){el.textContent=String(e)}}
async function saveBleTarget(){const address=document.getElementById('ble-target').value;try{const r=await fetch('/api/ble/tracking',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({address})});const d=await r.json();document.getElementById('ble-tracking').textContent=JSON.stringify(d,null,2)}catch(e){document.getElementById('ble-tracking').textContent=String(e)}}
async function clearBleTarget(){document.getElementById('ble-target').value='';saveBleTarget()}
async function loadBleTarget(){try{const d=await (await fetch('/api/ble/tracking')).json();document.getElementById('ble-target').value=d.address||'';document.getElementById('ble-tracking').textContent=JSON.stringify(d,null,2)}catch(e){}}
async function sendIr(){try{const a=document.getElementById('ir-address').value,c=document.getElementById('ir-command').value;if(!a.trim()||!c.trim())throw Error('Enter an IR address and command');const address=Number(a),command=Number(c);if(!Number.isInteger(address)||!Number.isInteger(command)||address<0||address>65535||command<0||command>255)throw Error('Address 0–65535; command 0–255');const r=await fetch('/api/ir/nec',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({address,command})});const d=await r.json();document.getElementById('ir-result').textContent=JSON.stringify(d,null,2)}catch(e){document.getElementById('ir-result').textContent=String(e)}}
async function uploadBrandLogo(){const f=document.getElementById('logo-file').files[0];if(!f||f.type!=='image/png'||f.size>150000){document.getElementById('logo-result').textContent='Select a PNG under 150 KB';return}const data=new FormData();data.append('file',f);const r=await fetch('/api/logo',{method:'POST',body:data});document.getElementById('logo-result').textContent=JSON.stringify(await r.json());document.querySelector('.hero img').src='/logo.png?t='+Date.now()}
async function statusNow(){try{const r=await fetch('/api/status');if(!r.ok)throw Error('HTTP '+r.status);document.getElementById('status').textContent=JSON.stringify(await r.json(),null,2);document.querySelector('.status-dot').style.background='#52dba5'}catch(e){document.getElementById('status').textContent='Disconnected';document.querySelector('.status-dot').style.background='#f46b78'}}
async function scan(){const el=document.getElementById('networks');try{const r=await fetch('/api/wifi/scan',{method:'POST'});if(!r.ok)throw Error(await r.text());el.textContent='Scanning...';poll()}catch(e){el.textContent=String(e)}}
async function poll(){try{const r=await fetch('/api/wifi/results');const d=await r.json();if(d.state==='scanning'){setTimeout(poll,1100);return}document.getElementById('networks').textContent=d.state==='complete'?(d.networks||[]).map(n=>n.ssid+'  '+n.rssi+' dBm  CH '+n.channel).join('\\n')||'No networks found':JSON.stringify(d)}catch(e){document.getElementById('networks').textContent=String(e)}}
async function signalWifi(){try{const d=await (await fetch('/api/signal/wifi')).json();document.getElementById('signal-wifi').textContent=(d.signals||[]).map(n=>n.ssid+' ('+n.bssid+') '+n.rssi+' dBm').join('\\n')||'Run a Wi-Fi scan first'}catch(e){document.getElementById('signal-wifi').textContent=String(e)}}
async function pathShieldStatus(){try{const d=await (await fetch('/api/pathshield')).json();document.getElementById('pathshield').checked=!!d.enabled;document.getElementById('pathshield-state').textContent=d.enabled?('Enabled — '+d.alert):'Disabled'}catch(e){document.getElementById('pathshield-state').textContent=String(e)}}
async function setPathShield(enabled){try{const r=await fetch('/api/pathshield',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled})});if(!r.ok)throw Error(await r.text());pathShieldStatus()}catch(e){document.getElementById('pathshield-state').textContent=String(e)}}
fetch('/logo.png',{method:'HEAD'}).then(r=>{if(r.ok)document.getElementById('skb-emblem').src='/logo.png'}).catch(()=>{});statusNow();pathShieldStatus();loadBleTarget();setInterval(statusNow,5000);
</script></html>
)SKBHTML");
  });
  server.on("/logo.png", HTTP_GET, []() {
    if(!filesystemReady || !LittleFS.exists("/logo.png")) {server.send(404,"text/plain","Upload SKB logo PNG using the dashboard");return;}
    File logo=LittleFS.open("/logo.png","r");server.streamFile(logo,"image/png");logo.close();
  });
  server.onNotFound([](){server.send(404,"application/json","{\"ok\":false,\"error\":\"not_found\"}");});
  server.begin();
}

void loop() {
  M5.update();
  server.handleClient();
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      onSerialLine(input);
      input = "";
    } else if (c != '\r' && input.length() < 512) {
      input += c;
    }
  }
  delay(2);
}
