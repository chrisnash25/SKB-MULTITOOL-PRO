#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <M5Unified.h>

static WebServer server(80);
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
  server.on("/api/status", HTTP_GET, sendStatus);
  server.on("/api/wifi/scan", HTTP_POST, startWifiScan);
  server.on("/api/wifi/results", HTTP_GET, wifiResults);
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", "<!doctype html><meta name='viewport' content='width=device-width,initial-scale=1'><style>body{background:#090d14;color:#e7edf8;font-family:sans-serif;padding:20px}h1{color:#38bdf8}</style><h1>SKB MULTITOOL PRO</h1><pre id='s'>Loading...</pre><script>async function tick(){try{let r=await fetch('/api/status');document.getElementById('s').textContent=JSON.stringify(await r.json(),null,2)}catch(e){document.getElementById('s').textContent='Disconnected'}}tick();setInterval(tick,3000)</script>");
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
