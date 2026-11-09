#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <M5Unified.h>
#include <Preferences.h>

static WebServer server(80);
static Preferences skbPrefs;
static bool pathShieldEnabled = false;
static unsigned long pathShieldEnabledSince = 0;

void sendPathShieldStatus() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["enabled"] = pathShieldEnabled;
  doc["scanner_ready"] = false;
  doc["mode"] = "configuration_only";
  doc["note"] = "BLE persistence detection and alerts are not yet integrated";
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
    WiFi.scanDelete();
  }
  doc["ok"] = true;
  String json; serializeJson(doc,json); server.send(200,"application/json",json);
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

void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  M5.begin(cfg);
  skbPrefs.begin("skb-settings",false);
  pathShieldEnabled=skbPrefs.getBool("pathshield",false);
  // Headless-first: no screen interaction required.\n  M5.Display.setRotation(0);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(8, 20);
  M5.Display.println("SKB MULTITOOL");
  M5.Display.setTextSize(1);
  M5.Display.println("  PRO");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apName);
  server.on("/api/status", HTTP_GET, sendStatus);
  server.on("/api/pathshield", HTTP_GET, sendPathShieldStatus);
  server.on("/api/pathshield", HTTP_POST, setPathShieldEnabled);
  server.on("/api/wifi/scan", HTTP_POST, startWifiScan);
  server.on("/api/wifi/results", HTTP_GET, wifiResults);
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", R"SKBHTML(
<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>SKB MULTITOOL PRO</title>
<style>
*{box-sizing:border-box}body{margin:0;background:#080c12;color:#eaf1fa;font:15px system-ui,Arial}
main{max-width:560px;margin:auto;padding:17px}.hero,.panel{background:linear-gradient(145deg,#192230,#10151e);border:1px solid #2a4055;border-radius:20px;padding:18px;margin-bottom:13px}
.hero{text-align:center}.hero img{width:112px;height:112px;object-fit:contain}h1{font-size:22px;margin:7px 0;color:#dce9f7}
h2{font-size:18px;margin:4px 0 7px}small,.muted{color:#91a8bc}.tag{color:#69d5ff;font-size:12px;letter-spacing:1.5px}
button{background:#123a50;border:1px solid #34bfff;border-radius:12px;color:#e9f8ff;font-weight:700;padding:12px 15px;cursor:pointer}
button:disabled{opacity:.45}.grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}.grid .panel{margin:0}
pre{white-space:pre-wrap;overflow-wrap:anywhere;font-size:12px;color:#a9dcf7}a{color:#7bdcff}
</style><main>
<header class="hero"><img src="/logo.png" alt="Original SKB emblem" onerror="this.style.display='none'">
<h1>SKB MULTITOOL PRO</h1><div class="tag">HEADLESS CONTROL CONSOLE</div><p class="muted">M5StickC Plus2 • Samsung Galaxy S21</p></header>
<section class="panel"><div class="tag">DEVICE</div><h2>System status</h2><pre id="status">Connecting...</pre><button onclick="statusNow()">Refresh</button></section>
<section class="panel"><div class="tag">INFRARED</div><h2>TV-B-Gone / Monitor-B-Gone</h2><p class="muted">IR code catalogue and controlled power profiles are being integrated. No transmission is enabled on this build.</p><button disabled>TV library — pending</button> <button disabled>Monitor library — pending</button></section>
<div class="grid"><section class="panel"><div class="tag">WI-FI</div><h2>Network scan</h2><button onclick="scan()">Scan</button></section>
<section class="panel"><div class="tag">BLUETOOTH</div><h2>BLE discovery</h2><p class="muted">Firmware integration pending</p><button disabled>Discover</button></section></div>
<section class="panel"><div class="tag">SETTINGS / PRIVACY</div><h2>PathShield</h2><p class="muted">Anti-tracking mode configuration. Detection and alerts are not yet available.</p><label><input type="checkbox" id="pathshield" onchange="setPathShield(this.checked)"> Enable PathShield mode</label><pre id="pathshield-state">Loading...</pre></section>
<section class="panel"><h2>Nearby Wi-Fi</h2><pre id="networks">Tap Scan to discover networks.</pre></section>
<p class="muted">Access point: SKB-MULTITOOL-PRO • http://192.168.4.1</p>
</main><script>
async function statusNow(){try{const r=await fetch('/api/status');document.getElementById('status').textContent=JSON.stringify(await r.json(),null,2)}catch(e){document.getElementById('status').textContent='Disconnected'}}
async function scan(){const el=document.getElementById('networks');try{const r=await fetch('/api/wifi/scan',{method:'POST'});if(!r.ok)throw Error(await r.text());el.textContent='Scanning...';poll()}catch(e){el.textContent=String(e)}}
async function poll(){try{const r=await fetch('/api/wifi/results');const d=await r.json();if(d.state==='scanning'){setTimeout(poll,1100);return}document.getElementById('networks').textContent=d.state==='complete'?(d.networks||[]).map(n=>n.ssid+'  '+n.rssi+' dBm  CH '+n.channel).join('\\n')||'No networks found':JSON.stringify(d)}catch(e){document.getElementById('networks').textContent=String(e)}}
async function pathShieldStatus(){try{const d=await (await fetch('/api/pathshield')).json();document.getElementById('pathshield').checked=!!d.enabled;document.getElementById('pathshield-state').textContent=d.enabled?'Enabled in settings — scanner integration pending':'Disabled'}catch(e){document.getElementById('pathshield-state').textContent=String(e)}}
async function setPathShield(enabled){try{await fetch('/api/pathshield',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled})});pathShieldStatus()}catch(e){document.getElementById('pathshield-state').textContent=String(e)}}
statusNow();pathShieldStatus();setInterval(statusNow,5000);
</script></html>
)SKBHTML");
  });
  server.on("/logo.png", HTTP_GET, []() {
    server.send(404, "text/plain", "Logo asset not yet installed in firmware");
  });
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
