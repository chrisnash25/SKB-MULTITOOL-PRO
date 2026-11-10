#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <M5Unified.h>
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertising.h>
#include <IRremote.hpp>
#include <LittleFS.h>
#include "skb_ir_library.h"
#include "skb_ir_presets.h"
#include "skb_ir_monitor_presets.h"

#include "mambo_radio_lab.h"
#include "mambo_radio_diagnostics.h"
#include "mambo_radio_probe.h"
static Preferences skbPrefs;
static WebServer server(80);
static String mamboRadioMode = "none";
static void mamboRadioStatus() {
  JsonDocument d;
  MamboRadio::fillDiagnostics(d,mamboRadioMode);
  String out;serializeJson(d,out);server.send(200,"application/json",out);
}

static void mamboRadioSelect() {
  JsonDocument request;
  if(deserializeJson(request,server.arg("plain"))) {server.send(400,"application/json",R"({"ok":false,"error":"invalid_json"})");return;}
  String mode=request["mode"] | "";
  if(mode!="none" && mode!="cc1101" && mode!="nrf24") {server.send(400,"application/json",R"({"ok":false,"error":"invalid_mode"})");return;}
  mamboRadioMode=mode;
  skbPrefs.putString("mambo_radio",mode);
  mamboRadioStatus();
}
static void mamboSubInspect() {
  String body=server.arg("plain");
  if(body.length()>8192) {server.send(413,"application/json",R"({"ok":false,"error":"file_too_large"})");return;}
  if(!body.startsWith("Filetype: Flipper SubGhz")) {server.send(400,"application/json",R"({"ok":false,"error":"unsupported_sub_header"})");return;}
  JsonDocument d;d["ok"]=true;d["type"]="sub_metadata_preview";d["transmission_enabled"]=false;
  const char* keys[]={"Version","Frequency","Preset","Protocol","Bit"};
  int cursor=0;
  while(cursor<body.length()) {
    int end=body.indexOf('\n',cursor);
    if(end<0)end=body.length();
    String line=body.substring(cursor,end);
    line.trim();
    for(const char* key:keys) {
      String prefix=String(key)+":";
      if(line.startsWith(prefix)) {
        String value=line.substring(prefix.length());value.trim();
        if(value.length()>100)value=value.substring(0,100);
        d[key]=value;
      }
    }
    cursor=end+1;
  }
  String out;serializeJson(d,out);server.send(200,"application/json",out);
}

static constexpr uint8_t IR_PIN = 19;
static bool filesystemReady = false;
static File logoUploadFile;
static BLEScan* bleScanner = nullptr;
static BLEAdvertising* bleAdvertiser = nullptr;
static bool bleAdvertisingEnabled = false;
static String trackedBleAddress;
static bool trackedBleSeen = false;
static int trackedBleRssi = -127;
static unsigned long trackedBleLastSeenMs = 0;
static bool pathShieldEnabled = false;
static unsigned int trackerCandidatesLastScan = 0;
static String trackerAlert = "No BLE scan yet";
static unsigned long pathShieldEnabledSince = 0;


#include "skb_badble.h"

void bleAdvertisementControl() {
 if(server.method()==HTTP_POST) {server.send(409,"application/json",R"({"ok":false,"error":"BLE_HID_keyboard_owns_advertising","hint":"BLE HID pairing runs automatically"})");return;}
 if(server.method()==HTTP_POST) {
  JsonDocument request;
  if(deserializeJson(request,server.arg("plain")) || !request["enabled"].is<bool>()) {server.send(400,"application/json",R"({"ok":false,"error":"enabled_boolean_required"})");return;}
  if(!bleAdvertiser){server.send(503,"application/json",R"({"ok":false,"error":"ble_advertiser_unavailable"})");return;}
  bool enabled=request["enabled"].as<bool>();
  if(enabled) bleAdvertiser->start();
  else bleAdvertiser->stop();
  bleAdvertisingEnabled=enabled;
 }
 JsonDocument result;result["ok"]=true;result["enabled"]=bleAdvertisingEnabled;result["name"]="SKB-MULTITOOL-PRO";result["mode"]="device_discovery";result["note"]="Advertises SKB device identity only";String output;serializeJson(result,output);server.send(200,"application/json",output);
}
void preloadIRRemotes() {
 if(!filesystemReady)return;
 LittleFS.mkdir("/ir");
 // Presets are derived from Flipper-IRDB Samsung/Samsung.ir and LG/LG.ir power entries.
 const char* samsung="Filetype: IR signals file\nVersion: 1\nname: Power\ntype: parsed\nprotocol: SAMSUNG32\naddress: 0x07\ncommand: 0x02\n";
 const char* lg="Filetype: IR signals file\nVersion: 1\nname: Power\ntype: parsed\nprotocol: NEC\naddress: 0x04\ncommand: 0x08\n";
 if(!LittleFS.exists("/ir/Samsung_TV_Power.ir")){File file=LittleFS.open("/ir/Samsung_TV_Power.ir","w");if(file){file.print(samsung);file.close();}}

 // Samsung H6300 common-code fallback; compatibility with LE32 is not confirmed.
 const char* samsungClassic="Filetype: IR signals file\nVersion: 1\n#\nname: Power\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 02 00 00 00\n#\nname: 1\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 04 00 00 00\n#\nname: 2\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 05 00 00 00\n#\nname: 3\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 06 00 00 00\n#\nname: 4\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 08 00 00 00\n#\nname: 5\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 09 00 00 00\n#\nname: 6\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 0A 00 00 00\n#\nname: 7\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 0C 00 00 00\n#\nname: 8\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 0D 00 00 00\n#\nname: 9\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 0E 00 00 00\n#\nname: 0\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 11 00 00 00\n#\nname: Vol_up\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 07 00 00 00\n#\nname: Vol_dn\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 0B 00 00 00\n#\nname: Mute\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 0F 00 00 00\n#\nname: Ch_next\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 12 00 00 00\n#\nname: Ch_prev\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 10 00 00 00\n#\nname: Menu\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 1A 00 00 00\n#\nname: Guide\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 4F 00 00 00\n#\nname: Ch_list\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 6B 00 00 00\n#\nname: Pre_ch\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 13 00 00 00\n#\nname: Ttx_mix\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 2C 00 00 00\n#\nname: Play\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 47 00 00 00\n#\nname: Pause\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 4A 00 00 00\n#\nname: Stop\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 46 00 00 00\n#\nname: Forwards\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 48 00 00 00\n#\nname: Backwards\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 45 00 00 00\n#\nname: Record\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 49 00 00 00\n#\nname: P_size\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 3E 00 00 00\n#\nname: Ad_subt\ntype: parsed\nprotocol: Samsung32\naddress: 07 00 00 00\ncommand: 25 00 00 00\n";
 if(!LittleFS.exists("/ir/Samsung_Classic_Experimental.ir")){File file=LittleFS.open("/ir/Samsung_Classic_Experimental.ir","w");if(file){file.print(samsungClassic);file.close();}}
 if(!LittleFS.exists("/ir/LG_TV_Power.ir")){File file=LittleFS.open("/ir/LG_TV_Power.ir","w");if(file){file.print(lg);file.close();}}
}

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

#include "skb_usb_protocol.h"

void onSerialLine(const String& line) {
  JsonDocument request;
  JsonDocument response;
  DeserializationError error = deserializeJson(request, line);
  if (error) {
    Serial.println("{\"ok\":false,\"error\":\"invalid_json\"}");
    return;
  }
  if (skbUsbHandle(request,response)) {serializeJson(response,Serial);Serial.println();return;}
  String cmd = request["cmd"] | "";
  if (cmd == "status") {
    response["ok"] = true;
    response["device"] = "SKB MULTITOOL PRO";
    response["uptime_ms"] = millis();
    response["free_heap"] = ESP.getFreeHeap();
    response["battery_voltage"] = M5.Power.getBatteryVoltage();
    response["protocol"]="mambo-usb-v1";
    response["radio_mode"]=mamboRadioMode;
    response["hardware_detected"]=false;
  } else if (cmd == "radio_status") {
    MamboRadio::fillDiagnostics(response,mamboRadioMode);
  } else if (cmd == "radio_probe") {
    MamboRadio::probeReadOnly(response,mamboRadioMode);
  } else if (cmd == "radio_select") {
    String mode=request["mode"] | "";
    if(mode=="none" || mode=="cc1101" || mode=="nrf24") {
      mamboRadioMode=mode;
      skbPrefs.putString("mambo_radio",mode);
      response["ok"]=true;
      response["selected_mode"]=mamboRadioMode;
    } else {
      response["ok"]=false;
      response["error"]="invalid_mode";
    }
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

// Verified against chrisnash25/Flipper-IRDB:
// TVs/Samsung/Samsung.ir: Samsung32 address 07 00 00 00, command 02 00 00 00
// TVs/LG/LG.ir: NEC address 04 00 00 00, command 08 00 00 00
// A single selected brand is transmitted per request; there is no multi-brand broadcast.
void sendTVPower() {
 JsonDocument req;
 if(deserializeJson(req,server.arg("plain")) || !req["confirm"].is<bool>() || !req["confirm"].as<bool>()) {
  JsonDocument err;err["ok"]=false;err["error"]="confirmation_required";String payload;serializeJson(err,payload);server.send(400,"application/json",payload);return;
 }
 String brand=req["brand"] | "";
 if(brand=="Samsung") IrSender.sendSamsung(0x07,0x02,0);
 else if(brand=="LG") IrSender.sendNEC(0x04,0x08,0);
 else {JsonDocument err;err["ok"]=false;err["error"]="brand_not_yet_verified";String payload;serializeJson(err,payload);server.send(422,"application/json",payload);return;}
 JsonDocument out;out["ok"]=true;out["brand"]=brand;out["command"]="power_toggle";out["note"]="IR transmission attempted; TV state cannot be verified";String json;serializeJson(out,json);server.send(200,"application/json",json);
}

void listIRLibrary() {
 JsonDocument doc;doc["ok"]=filesystemReady;JsonArray files=doc["files"].to<JsonArray>();
 if(filesystemReady){File root=LittleFS.open("/ir");if(root){File file=root.openNextFile();while(file){if(!file.isDirectory()&&String(file.name()).endsWith(".ir")){JsonObject item=files.add<JsonObject>();item["name"]=String(file.name());item["bytes"]=file.size();}file=root.openNextFile();}}}
 String output;serializeJson(doc,output);server.send(filesystemReady?200:503,"application/json",output);
}
void importIRFile() {
 if(!filesystemReady){server.send(503,"application/json","{\"ok\":false,\"error\":\"storage_unavailable\"}");return;}
 String name=server.arg("name");if(name.length()<1||name.length()>55||!name.endsWith(".ir")){server.send(400,"application/json","{\"ok\":false,\"error\":\"invalid_name\"}");return;}
 for(size_t i=0;i<name.length();i++){char c=name[i];if(!isalnum((unsigned char)c)&&c!='_'&&c!='-'&&c!='.'){server.send(400,"application/json","{\"ok\":false,\"error\":\"invalid_name\"}");return;}}
 String body=server.arg("plain");std::vector<SKBIR::Signal> signals;String error;
 if(!SKBIR::parse(body,signals,error)){JsonDocument doc;doc["ok"]=false;doc["error"]=error;String out;serializeJson(doc,out);server.send(422,"application/json",out);return;}
 LittleFS.mkdir("/ir");File file=LittleFS.open("/ir/"+name,"w");if(!file){server.send(507,"application/json","{\"ok\":false,\"error\":\"storage_full\"}");return;}
 size_t written=file.print(body);file.close();JsonDocument doc;doc["ok"]=written==body.length();doc["signals"]=signals.size();String out;serializeJson(doc,out);server.send(written==body.length()?201:507,"application/json",out);
}
void sendStoredIR() {
 JsonDocument request;if(deserializeJson(request,server.arg("plain"))){server.send(400,"application/json","{\"ok\":false,\"error\":\"invalid_json\"}");return;}
 String name=request["file"] | "";String key=request["key"] | "";if(name.isEmpty()||name.indexOf('/')>=0||name.indexOf('\\')>=0||name.indexOf("..")>=0||!name.endsWith(".ir")){server.send(400,"application/json","{\"ok\":false,\"error\":\"invalid_file\"}");return;}
 File file=LittleFS.open("/ir/"+name,"r");if(!file){server.send(404,"application/json","{\"ok\":false,\"error\":\"not_found\"}");return;}
 if(file.size()>32000){file.close();server.send(413,"application/json","{\"ok\":false,\"error\":\"too_large\"}");return;}
 String body=file.readString();file.close();std::vector<SKBIR::Signal> signals;String error;
 if(!SKBIR::parse(body,signals,error)){server.send(422,"application/json","{\"ok\":false,\"error\":\"invalid_ir_file\"}");return;}
 for(auto &signal:signals){if(signal.name==key){bool ok=SKBIR::send(signal,error);JsonDocument doc;doc["ok"]=ok;if(!ok)doc["error"]=error;String out;serializeJson(doc,out);server.send(ok?200:422,"application/json",out);return;}}
 server.send(404,"application/json","{\"ok\":false,\"error\":\"key_not_found\"}");
}


void inspectStoredIR() {
 String name=server.arg("file");
 if(name.isEmpty()||name.indexOf('/')>=0||name.indexOf('\\')>=0||name.indexOf("..")>=0||!name.endsWith(".ir")){server.send(400,"application/json","{\"ok\":false,\"error\":\"invalid_file\"}");return;}
 File file=LittleFS.open("/ir/"+name,"r");if(!file){server.send(404,"application/json","{\"ok\":false,\"error\":\"not_found\"}");return;}
 if(file.size()>32000){file.close();server.send(413,"application/json","{\"ok\":false,\"error\":\"too_large\"}");return;}
 String body=file.readString();file.close();std::vector<SKBIR::Signal> signals;String error;
 if(!SKBIR::parse(body,signals,error)){server.send(422,"application/json","{\"ok\":false,\"error\":\"invalid_ir_file\"}");return;}
 JsonDocument doc;doc["ok"]=true;JsonArray keys=doc["keys"].to<JsonArray>();for(const auto &signal:signals){JsonObject key=keys.add<JsonObject>();key["name"]=signal.name;key["protocol"]=signal.protocol;}
 String out;serializeJson(doc,out);server.send(200,"application/json",out);
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(1000); // USB serial control transport
  auto cfg = M5.config();
  M5.begin(cfg);
  skbPrefs.begin("skb-settings",false);
  filesystemReady=LittleFS.begin(true);
  preloadIRRemotes();
  if(filesystemReady){installBundledIRProfiles();installMonitorPowerPresets();}
  IrSender.begin(IR_PIN);
  pathShieldEnabled=skbPrefs.getBool("pathshield",false);
  trackedBleAddress=skbPrefs.getString("ble_target","");
  mamboRadioMode=skbPrefs.getString("mambo_radio","none");
  if(mamboRadioMode!="none" && mamboRadioMode!="cc1101" && mamboRadioMode!="nrf24")mamboRadioMode="none";
  // Headless-first: no screen interaction required.
  M5.Display.setRotation(0);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(8, 20);
  M5.Display.println("MAMBO OS");
  M5.Display.setTextSize(1);
  M5.Display.println("  FIRMWARE");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apName);
  skbBadBleInit();
  bleScanner = BLEDevice::getScan();
  bleScanner->setActiveScan(false);
  bleAdvertiser=BLEDevice::getAdvertising();
  bleAdvertiser->setScanResponse(true);
  server.on("/api/status", HTTP_GET, sendStatus);
  server.on("/api/mambo/radio", HTTP_GET, mamboRadioStatus);
  server.on("/api/mambo/diagnostics", HTTP_GET, mamboRadioStatus);
  server.on("/api/mambo/probe", HTTP_POST, []() {
    JsonDocument d;MamboRadio::probeReadOnly(d,mamboRadioMode);
    String out;serializeJson(d,out);
    server.send(d["ok"].as<bool>()?200:503,"application/json",out);
  });
  server.on("/api/mambo/radio", HTTP_POST, mamboRadioSelect);
  server.on("/api/mambo/sub/inspect", HTTP_POST, mamboSubInspect);
  server.on("/api/ir/nec", HTTP_POST, sendIR);
  server.on("/api/ir/tv-power", HTTP_POST, sendTVPower);
  server.on("/api/ir/library", HTTP_GET, listIRLibrary);
  server.on("/api/ir/inspect", HTTP_GET, inspectStoredIR);
  server.on("/api/ir/import", HTTP_POST, importIRFile);
  server.on("/api/ir/send", HTTP_POST, sendStoredIR);
  server.on("/api/logo", HTTP_POST, [](){server.send(filesystemReady?200:503,"application/json",filesystemReady?"{\"ok\":true}":"{\"ok\":false}");}, uploadLogo);
  server.on("/api/ble/scan", HTTP_GET, scanBle);
  server.on("/api/ble/advertise", HTTP_GET, bleAdvertisementControl);
  server.on("/api/ble/advertise", HTTP_POST, bleAdvertisementControl);
  server.on("/api/badble", HTTP_GET, [](){JsonDocument r;skbBadBleStatus(r);String out;serializeJson(r,out);server.send(200,"application/json",out);});
  server.on("/api/badble", HTTP_POST, [](){JsonDocument req,res;if(deserializeJson(req,server.arg("plain"))){server.send(400,"application/json",R"({"ok":false,"error":"invalid_json"})");return;}skbBadBleCommand(req,res);String out;serializeJson(res,out);server.send(res["ok"].as<bool>()?200:422,"application/json",out);});
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
<title>Mambo OS Firmware</title>
<style>
:root{color-scheme:dark;--ink:#090909;--panel:#171717;--line:#4d3636;--blue:#bb3434;--silver:#ded8d5}
*{box-sizing:border-box}body{margin:0;color:#eee8e5;background:repeating-linear-gradient(115deg,#0b0b0c 0px,#101011 3px,#0b0b0c 6px);font:14px/1.5 system-ui,Arial}
main{max-width:640px;margin:auto;padding:12px 12px 90px}.hero,.panel{background:linear-gradient(150deg,#292929,#111112 62%,#090909);border:1px solid #534747;border-radius:9px;padding:15px;margin-bottom:12px;box-shadow:inset 0 1px #ffffff20,0 6px 18px #0009}
.hero{text-align:center;border-top:2px solid #9d3030}.hero img{width:115px;height:115px;object-fit:contain}.hero:after{content:"SECURE HARDWARE SOLUTIONS  •  MADE BY CHRIS NASH";display:block;color:#bdb4b4;font-size:10px;letter-spacing:1.2px;margin-top:8px}
h1{font-size:23px;letter-spacing:1px;margin:10px 0}h2{font-size:17px;margin:3px 0 12px}small,.muted{color:#ada6a6}.tag{color:#d44b4b;font-size:11px;letter-spacing:1.5px;font-weight:800}
button{background:linear-gradient(#444,#222 48%,#111);color:#f1eeee;border:1px solid #746666;border-bottom:2px solid #8e2929;border-radius:7px;padding:12px;min-height:44px;font-weight:750;box-shadow:inset 0 1px #ffffff2c;cursor:pointer}
button:active{background:#572222}button:focus-visible,input:focus-visible{outline:2px solid #d24c4c;outline-offset:2px}
input{max-width:100%;padding:10px;background:#0b0b0c;color:white;border:1px solid #675050;border-radius:5px}input[type=number],input[type=text],input:not([type]){width:100%;margin:5px 0 10px}input[type=file]{display:block;width:100%;margin:10px 0}input[type=checkbox]{accent-color:#b63232}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.grid .panel{margin:0 0 10px}.grid button{width:100%}
pre{white-space:pre-wrap;overflow-wrap:anywhere;background:#080809;border:1px solid #483535;border-radius:5px;color:#d7c5c5;padding:12px;max-height:320px;overflow:auto;font:12px/1.6 monospace}
a{color:#e46d6d}.nav{display:flex;gap:7px;overflow:auto;padding:0 0 14px}.nav a{flex:none;text-decoration:none;background:linear-gradient(#292929,#111);border:1px solid #594444;border-bottom:2px solid #a12c2c;color:#eee;padding:9px;border-radius:5px;font-size:12px;font-weight:700}
.status-dot{display:inline-block;width:7px;height:7px;border-radius:50%;background:#6aa876;margin-right:5px}
@media(max-width:360px){.grid{grid-template-columns:1fr}}

/* Mobile-first paged controller: no endless scrolling between tools. */
html,body{height:100%;overflow:hidden}
main{height:100vh;height:100dvh;max-width:640px;display:flex;flex-direction:column;overflow:hidden;padding:8px 10px 10px}
.skb-tabs{display:flex;gap:6px;overflow-x:auto;flex:none;padding:4px 2px 9px;scrollbar-width:thin}
.skb-tabs button{flex:none;white-space:nowrap;min-height:42px;padding:9px 14px;border-radius:6px;font-size:12px;letter-spacing:.15px}
.skb-tabs button[aria-selected="true"]{background:linear-gradient(#7e2020,#451515);border-color:#e66b6b;color:#fff}
.skb-pages{flex:1;min-height:0;overflow:hidden}
.skb-page{display:none;height:100%;overflow-y:auto;overscroll-behavior:contain;touch-action:pan-y;padding:0 2px 18px;scrollbar-width:thin}
.skb-page.skb-active{display:block}
.skb-page-title{position:sticky;top:0;z-index:2;background:linear-gradient(90deg,#261717,#101011);border-bottom:1px solid #6b2929;padding:8px 10px;margin:0 0 10px;display:flex;align-items:center;justify-content:space-between;gap:10px}
.skb-page-title strong{font-size:16px;letter-spacing:.7px}
.skb-page-title span{font-size:11px;color:#cfa0a0}
.skb-pager{flex:none;display:flex;align-items:center;gap:9px;justify-content:space-between;border-top:1px solid #6a3333;padding:9px 2px 0;background:#0b0b0c}
.skb-pager button{min-width:97px;min-height:48px}
.skb-pager button:disabled{opacity:.34;cursor:default}
.skb-page-indicator{text-align:center;flex:1;font-size:12px;color:#e6dddd;font-weight:700}
.skb-page-indicator small{display:block;color:#b5a4a4;font-size:10px;font-weight:400}
.skb-page .panel{margin-bottom:12px}
.skb-page select{max-width:100%}
@media(max-width:360px){.skb-pager button{min-width:80px}.skb-tabs button{padding:8px 11px}}
</style><main><section class="panel" id="tv-power-panel"><h2>TV POWER CONTROL</h2><p class="muted">Select your own TV brand. One power-toggle signal per tap.</p><label for="tv-brand">Verified TV brand</label><select id="tv-brand" style="width:100%;padding:12px;background:#151515;color:white;border:1px solid #794747;border-radius:6px"><option value="Samsung">Samsung</option><option value="LG">LG</option></select><button type="button" id="tv-power-button" style="display:block;width:100%;margin-top:12px">POWER TOGGLE</button><p id="tv-power-result" role="status" class="muted">No command sent</p></section>
<script>
document.getElementById('tv-power-button').addEventListener('click',async()=>{
const brand=document.getElementById('tv-brand').value;

const status=document.getElementById('tv-power-result');status.textContent='Sending…';
try{const response=await fetch('/api/ir/tv-power',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({brand,confirm:true})});const data=await response.json();status.textContent=data.ok?'IR sent (TV state not confirmed)':('Error: '+(data.error||response.status));}
catch(e){status.textContent='Connection failed: '+e.message;}
});
</script>

<header class="hero"><img id="skb-emblem" src="data:image/jpeg;base64,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" alt="Original stitched SKB emblem">
<h1>SKB MULTITOOL PRO</h1><div><label class="muted">Customise emblem · stitched SKB PNG (max 150 KB) <input id="logo-file" type="file" accept="image/png"></label><button onclick="uploadBrandLogo()">Apply logo</button><small id="logo-result"></small></div><div class="tag">HEADLESS CONTROL CONSOLE</div><p class="muted">M5StickC Plus2 / Mobile command centre</p></header>
<nav class="nav"><a href="#device">System</a><a href="#infrared">Infrared</a><a href="#bluetooth">Bluetooth</a><a href="#privacy">PathShield</a><a href="#wifi">Wi-Fi</a></nav><section id="device" class="panel"><div class="tag"><span class="status-dot"></span>DEVICE / STATUS</div><h2>System status</h2><pre id="status">Connecting...</pre><button onclick="statusNow()">Refresh</button></section>
<section class="panel" id="ble-advertising"><div class="tag">BLUETOOTH / TX</div><h2>SKB BLE Beacon</h2><p class="muted">Broadcast your own SKB-MULTITOOL-PRO identity. No impersonation or repeated packet flooding.</p><button type="button" onclick="setBleAdvertise(true)">Start advertising</button> <button type="button" onclick="setBleAdvertise(false)">Stop advertising</button><pre id="ble-advertise-status">Checking status...</pre></section>
<section class="panel" id="ir-presets"><div class="tag">IR LIBRARY</div><h2>Preloaded TV Remotes</h2><p class="muted">Verified Samsung and LG power-toggle commands. Select a file and tap a command. Additional compatible .ir files can be imported.</p><select id="ir-preset-file" style="width:100%;padding:10px;background:#111;color:#eee"></select><button type="button" onclick="loadIRKeys()">Load buttons</button><div id="ir-preset-buttons" class="grid"></div><pre id="ir-preset-status">Loading library...</pre><label>Import a compatible Flipper .ir file (up to 32 KB)<input type="file" id="ir-import-file" accept=".ir,text/plain"></label><button type="button" onclick="importIRFileUI()">Import file</button></section>
<section id="monitor-power" class="panel"><div class="tag">INFRARED / MONITORS</div><h2>Monitor power controls</h2><p class="muted">Eight model-specific power buttons from your Flipper-IRDB. Point the IR transmitter at your own monitor. Buttons transmit IR; they do not confirm whether the monitor changed state.</p><div id="monitor-power-buttons" class="grid"></div><pre id="monitor-power-status">Eight monitor power profiles ready when storage is mounted.</pre></section>
<script>
async function setBleAdvertise(enabled){const el=document.getElementById('ble-advertise-status');try{const r=await fetch('/api/ble/advertise',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled})});el.textContent=JSON.stringify(await r.json(),null,2)}catch(e){el.textContent=String(e)}}
async function bleAdvertiseStatus(){try{document.getElementById('ble-advertise-status').textContent=JSON.stringify(await (await fetch('/api/ble/advertise')).json(),null,2)}catch(e){}}
async function refreshIRFiles(){const select=document.getElementById('ir-preset-file');const out=document.getElementById('ir-preset-status');try{const r=await fetch('/api/ir/library');const d=await r.json();if(!r.ok)throw Error(JSON.stringify(d));select.replaceChildren();for(const f of d.files||[]){const opt=document.createElement('option');opt.value=f.name.split('/').pop();opt.textContent=opt.value;select.append(opt)}out.textContent=select.options.length+' remote file(s) ready';await loadIRKeys()}catch(e){out.textContent=String(e)}}
async function loadIRKeys(){const name=document.getElementById('ir-preset-file').value;const target=document.getElementById('ir-preset-buttons');target.replaceChildren();if(!name)return;try{const r=await fetch('/api/ir/inspect?file='+encodeURIComponent(name));const d=await r.json();if(!r.ok)throw Error(JSON.stringify(d));for(const k of d.keys||[]){const button=document.createElement('button');button.textContent=k.name;button.addEventListener('click',async()=>{const out=document.getElementById('ir-preset-status');out.textContent='Transmitting...';try{const response=await fetch('/api/ir/send',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({file:name,key:k.name})});const result=await response.json();out.textContent=result.ok?'IR transmitted; device response not verified':JSON.stringify(result)}catch(e){out.textContent=String(e)}});target.append(button)}}catch(e){document.getElementById('ir-preset-status').textContent=String(e)}}
async function importIRFileUI(){const file=document.getElementById('ir-import-file').files[0];if(!file)return;const out=document.getElementById('ir-preset-status');if(file.size>32000){out.textContent='File exceeds 32 KB';return}try{const content=await file.text();const r=await fetch('/api/ir/import?name='+encodeURIComponent(file.name),{method:'POST',headers:{'Content-Type':'text/plain'},body:content});const d=await r.json();if(!r.ok)throw Error(JSON.stringify(d));out.textContent='Imported '+file.name;await refreshIRFiles()}catch(e){out.textContent=String(e)}}
const monitorPowerPresets=[
['Aorus FV43U','Monitor_Aorus_FV43U.ir'],
['BenQ EX2780Q','Monitor_BenQ_EX2780Q.ir'],
['Clinton M-10A','Monitor_Clinton_Electronics_M10A.ir'],
['LG 43UD79-B','Monitor_LG_43UD79-B.ir'],
['LG UltraGear OLED','Monitor_LG_UltraGear_OLED.ir'],
['Loncevon 12-inch','Monitor_Loncevon_12inch.ir'],
['Monoprice 43305','Monitor_Monoprice_43305.ir'],
['Philips Evnia 49M2C8900L','Monitor_Philips_Evnia_49M2C8900L.ir']
];
function renderMonitorPower(){const target=document.getElementById('monitor-power-buttons');if(!target)return;target.replaceChildren();for(const [label,file] of monitorPowerPresets){const b=document.createElement('button');b.type='button';b.textContent=label+' · Power';b.addEventListener('click',()=>sendMonitorPower(file,label));target.append(b)}}
async function sendMonitorPower(file,label){const el=document.getElementById('monitor-power-status');el.textContent='Sending '+label+' Power...';try{const response=await fetch('/api/ir/send',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({file,key:'Power'})});const result=await response.json();el.textContent=response.ok&&result.ok?label+': Power IR transmitted (monitor state unverified)':label+': '+JSON.stringify(result)}catch(e){el.textContent=label+': '+String(e)}}
renderMonitorPower();bleAdvertiseStatus();refreshIRFiles();
</script><section id="infrared" class="panel"><div class="tag">INFRARED / TX</div><h2>NEC IR Remote</h2><p class="muted">Send a single NEC command to a device you own. Enter the address and command from its remote specification (decimal values).</p><input id="ir-address" type="number" min="0" max="65535" placeholder="Address (0–65535)"><input id="ir-command" type="number" min="0" max="255" placeholder="Command (0–255)"><button onclick="sendIr()">Transmit</button><pre id="ir-result">Ready</pre></section>
<div class="grid"><section class="panel"><div class="tag">WI-FI</div><h2>Network scan</h2><button onclick="scan()">Scan</button></section>
<section class="panel"><div class="tag">BLUETOOTH</div><h2>BLE discovery</h2><p class="muted">BLE advertisement discovery</p><button onclick="scanBle()">Discover</button></section></div>
<section id="bluetooth" class="panel"><div class="tag">BLUETOOTH RESULTS</div><pre id="ble-results">Tap Discover to scan BLE advertisements.</pre><h2>Track one of your devices</h2><input id="ble-target" placeholder="BLE address AA:BB:CC:DD:EE:FF" style="width:100%;padding:12px;background:#0b1723;color:#fff;border:1px solid #34bfff;border-radius:10px;margin:8px 0"><button onclick="saveBleTarget()">Save target</button> <button onclick="clearBleTarget()">Clear</button><pre id="ble-tracking">No target configured</pre></section>
<section class="panel" id="badble"><div class="tag">BLE HID / MANUAL MACROS</div><h2>SKB BLE Keyboard</h2>
<p class="muted">Pair once with SKB BLE HID on your own phone or PC. The keyboard advertises and reconnects automatically when the host permits it. Text macros NEVER run on connection; select one and press RUN.</p>
<button onclick="skbHidStatus()">CHECK PAIRING</button><pre id="skb-hid-status">Checking...</pre>
<label>Saved macro <select id="skb-hid-select" style="width:100%;padding:11px;background:#171717;color:white"></select></label>
<button onclick="skbHidLoad()">LOAD / REVIEW</button>
<label>Macro name <input id="skb-hid-name" maxlength="20" placeholder="MyMacro"></label>
<label>Macro text (printable characters only, max 160)<textarea id="skb-hid-text" maxlength="160" rows="4" style="width:100%;background:#131313;color:#fff"></textarea></label>
<div class="grid"><button onclick="skbHidSave()">SAVE</button><button onclick="skbHidDelete()">DELETE</button><button onclick="skbHidRun()">RUN SELECTED</button><button onclick="skbHidStop()">STOP</button></div>
<pre id="skb-hid-result">Only manually triggered text macros can transmit.</pre></section>
<script>
async function skbHid(action='status',fields={}){const r=await fetch('/api/badble',{method:action==='status'?'GET':'POST',headers:{'Content-Type':'application/json'},body:action==='status'?undefined:JSON.stringify({action,...fields})});const d=await r.json();if(!r.ok||!d.ok)throw Error(d.error||r.status);return d}
async function skbHidStatus(){try{const x=await skbHid();document.getElementById('skb-hid-status').textContent=x.connected?'Paired and connected; ready for selected macro':'Not connected. Pair SKB BLE Keyboard in the host Bluetooth settings first.';if(x.running)document.getElementById('skb-hid-status').textContent+=' Macro running: '+x.selected;}catch(e){document.getElementById('skb-hid-status').textContent=String(e)}}
async function skbHidList(){try{const d=await skbHid('list');const sel=document.getElementById('skb-hid-select');sel.replaceChildren();for(const m of d.macros){const o=document.createElement('option');o.value=m.name;o.textContent=m.name;o.dataset.text=m.text;sel.append(o)} }catch(e){document.getElementById('skb-hid-result').textContent=String(e)}}
function skbHidLoad(){const sel=document.getElementById('skb-hid-select'),o=sel.selectedOptions[0];if(!o)return;document.getElementById('skb-hid-name').value=o.value;document.getElementById('skb-hid-text').value=o.dataset.text||''}
async function skbHidSave(){const name=document.getElementById('skb-hid-name').value,text=document.getElementById('skb-hid-text').value;try{const d=await skbHid('save',{name,text});document.getElementById('skb-hid-result').textContent='Saved '+d.name;await skbHidList()}catch(e){document.getElementById('skb-hid-result').textContent=String(e)}}
async function skbHidDelete(){const name=document.getElementById('skb-hid-select').value;try{await skbHid('delete',{name});document.getElementById('skb-hid-result').textContent='Deleted '+name;await skbHidList()}catch(e){document.getElementById('skb-hid-result').textContent=String(e)}}
async function skbHidRun(){const name=document.getElementById('skb-hid-select').value;const out=document.getElementById('skb-hid-result');if(!name){out.textContent='Select a saved macro first';return}try{await skbHid('run',{name});out.textContent='RUN selected: '+name;skbHidStatus()}catch(e){out.textContent=String(e)}}
async function skbHidStop(){try{await skbHid('stop');document.getElementById('skb-hid-result').textContent='Stopped';skbHidStatus()}catch(e){document.getElementById('skb-hid-result').textContent=String(e)}}
skbHidList();skbHidStatus();
</script><section id="privacy" class="panel"><div class="tag">SETTINGS / PRIVACY</div><h2>PathShield</h2><p class="muted">BLE tracker-candidate classification and last-scan counts. These are not proof of unwanted tracking.</p><label><input type="checkbox" id="pathshield" onchange="setPathShield(this.checked)"> Enable PathShield mode</label><pre id="pathshield-state">Loading...</pre></section>
<section class="panel"><div class="tag">SIGNAL FINDER</div><h2>Wi-Fi strength</h2><p class="muted">Signal levels from completed Wi-Fi scans; refresh by scanning again. Live tracking and BLE proximity alerts are not yet available.</p><button onclick="signalWifi()">View signal levels</button><pre id="signal-wifi">No readings yet</pre></section>
<section id="wifi" class="panel"><h2>Nearby Wi-Fi</h2><pre id="networks">Tap Scan to discover networks.</pre></section>
<p class="muted">Access point: SKB-MULTITOOL-PRO • http://192.168.4.1</p>
<section class="panel" id="monitor-power"><h2>MONITOR POWER CONTROL</h2><p class="muted">Choose an imported monitor remote and its power command. Power transmits immediately when tapped; it may toggle rather than force OFF.</p><label>Monitor IR file <select id="monitor-file" style="width:100%;padding:12px;background:#111;color:#eee;border:1px solid #745454"></select></label><p><label>Power command <select id="monitor-key" style="width:100%;padding:12px;background:#111;color:#eee;border:1px solid #745454"></select></label></p><button type="button" id="monitor-refresh">REFRESH MONITOR REMOTES</button> <button type="button" id="monitor-send" disabled>POWER</button><pre id="monitor-result" aria-live="polite">No monitor remote selected</pre></section>
<script>
(()=>{const file=document.getElementById('monitor-file'),key=document.getElementById('monitor-key'),send=document.getElementById('monitor-send'),out=document.getElementById('monitor-result');const power=/^(power|power on.off|on.off|standby|pwr|power_toggle)$/i;
async function request(url,options){const response=await fetch(url,options);const data=await response.json();if(!response.ok||!data.ok)throw Error(data.error||'Request failed');return data;}
async function inspect(){send.disabled=true;key.replaceChildren();if(!file.value)return;try{const data=await request('/api/ir/inspect?file='+encodeURIComponent(file.value));for(const entry of data.keys.filter(x=>power.test(x.name.trim()))){const option=document.createElement('option');option.value=entry.name;option.textContent=entry.name+' ('+entry.protocol+')';key.append(option);}send.disabled=!key.options.length;out.textContent=key.options.length?'Power command ready. Tap POWER to transmit.':'No supported power key in this file.';}catch(e){out.textContent=e.message;}}
async function refresh(){file.replaceChildren();send.disabled=true;try{const data=await request('/api/ir/library');for(const entry of data.files){const name=entry.name.split('/').pop();const option=document.createElement('option');option.value=name;option.textContent=name;file.append(option);}await inspect();}catch(e){out.textContent=e.message;}}
file.addEventListener('change',inspect);document.getElementById('monitor-refresh').addEventListener('click',refresh);
send.addEventListener('click',async()=>{if(!file.value||!key.value)return;send.disabled=true;out.textContent='Transmitting...';try{const result=await request('/api/ir/send',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({file:file.value,key:key.value})});out.textContent=result.ok?'IR command transmitted (monitor response cannot be verified).':'Transmission failed';}catch(e){out.textContent='Error: '+e.message;}finally{send.disabled=false;}});
refresh();})();
</script></main><script>
(()=>{
const host=document.querySelector('main');if(!host)return;
const specs=[['radio','RADIO LAB'],['home','HOME'],['tv','TV REMOTES'],['monitors','MONITORS'],['wifi','WI-FI'],['bluetooth','BLUETOOTH'],['badble','BLE KEYBOARD'],['settings','SETTINGS']];
const containers={};
const deck=document.createElement('div');deck.className='skb-pages';deck.id='skb-pages';
const tabs=document.createElement('nav');tabs.className='skb-tabs';tabs.setAttribute('aria-label','Dashboard pages');
const footer=document.createElement('div');footer.className='skb-pager';
for(const [id,title] of specs){const pg=document.createElement('section');pg.className='skb-page';pg.id='skb-page-'+id;pg.setAttribute('aria-label',title);
const head=document.createElement('div');head.className='skb-page-title';const strong=document.createElement('strong');strong.textContent=title;const hint=document.createElement('span');hint.textContent='SWIPE LEFT / RIGHT';head.append(strong,hint);pg.append(head);containers[id]=pg;deck.append(pg);
const tab=document.createElement('button');tab.type='button';tab.textContent=title;tab.dataset.page=id;tab.setAttribute('aria-controls',pg.id);tab.addEventListener('click',()=>show(id));tabs.append(tab)}
const radioPanel=document.createElement('article');
radioPanel.innerHTML='<h2>Mambo OS V3 · Radio Lab</h2><p>One interchangeable 8-pin radio at a time. Switch off the M5StickC Plus2 before changing modules.</p><label>Selected module <select id="mambo-mode"><option value="none">No module</option><option value="cc1101">CC1101 Sub-GHz</option><option value="nrf24">nRF24 2.4 GHz</option></select></label> <button id="mambo-set">Set mode</button> <button id="mambo-refresh">Refresh</button><pre id="mambo-status"></pre><h3>Flipper .sub metadata inspector</h3><input type="file" id="mambo-sub" accept=".sub,text/plain"><button id="mambo-inspect">Inspect</button><pre id="mambo-sub-result"></pre><h3>Offline RF observation analyser</h3><p>Import an existing JSON array of observations: [{frequency_mhz:433.92,power_dbm:-72}] for CC1101, or [{channel:40,detected:true}] for nRF24. These are imported measurements, not live scans.</p><input type="file" id="mambo-observations" accept=".json,application/json"><button id="mambo-analyse">Analyse imported samples</button><pre id="mambo-observation-result"></pre><p>RF scanning and transmission remain unavailable until the module wiring and hardware are validated. Rolling-code support is limited to offline diagnostics.</p>';
containers.radio.append(radioPanel);
async function mamboRefresh(){try{const r=await fetch('/api/mambo/radio');const d=await r.json();document.getElementById('mambo-status').textContent=JSON.stringify(d,null,2);document.getElementById('mambo-mode').value=d.selected_mode||'none'}catch(e){document.getElementById('mambo-status').textContent=String(e)}}
document.getElementById('mambo-refresh').onclick=mamboRefresh;
document.getElementById('mambo-set').onclick=async()=>{await fetch('/api/mambo/radio',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode:document.getElementById('mambo-mode').value})});mamboRefresh()};
document.getElementById('mambo-inspect').onclick=async()=>{const file=document.getElementById('mambo-sub').files[0];if(!file)return;const output=document.getElementById('mambo-sub-result');if(file.size>8192){output.textContent='Maximum 8 KB';return}try{const r=await fetch('/api/mambo/sub/inspect',{method:'POST',headers:{'Content-Type':'text/plain'},body:await file.text()});output.textContent=JSON.stringify(await r.json(),null,2)}catch(e){output.textContent=String(e)}};
document.getElementById('mambo-analyse').onclick=async()=>{
 const f=document.getElementById('mambo-observations').files[0];
 const out=document.getElementById('mambo-observation-result');
 if(!f){out.textContent='Choose a JSON file';return}
 if(f.size>65536){out.textContent='File must be 64 KB or less';return}
 try {
  const rows=JSON.parse(await f.text());
  if(!Array.isArray(rows)||rows.length>512||!rows.length)throw Error('Expected 1 to 512 observations');
  const groups=new Map();
  for(const row of rows){
   if(!row||typeof row!=='object')throw Error('Invalid observation');
   const key=Number.isFinite(row.frequency_mhz)?'MHz '+Number(row.frequency_mhz).toFixed(3):
    Number.isInteger(row.channel)?'CH '+row.channel:null;
   if(!key)throw Error('Each observation needs frequency_mhz or channel');
   if(Number.isFinite(row.power_dbm)&& (row.power_dbm>30||row.power_dbm< -150))throw Error('Invalid power_dbm');
   if(!groups.has(key))groups.set(key,{samples:0,detected:0,powerSum:0,powerCount:0});
   const g=groups.get(key);g.samples++;
   if(row.detected===true)g.detected++;
   if(Number.isFinite(row.power_dbm)){g.powerSum+=row.power_dbm;g.powerCount++}
  }
  const lines=[...groups.entries()].sort((a,b)=>b[1].samples-a[1].samples).map(([key,g])=>
   key+' | samples '+g.samples+' | detections '+g.detected+
   (g.powerCount?' | mean '+(g.powerSum/g.powerCount).toFixed(1)+' dBm':''));
  out.textContent='OFFLINE IMPORT ONLY — '+rows.length+' observations\\n'+lines.join('\\n');
 } catch(e){out.textContent='Invalid observations: '+e.message}
};
mamboRefresh();
const original=Array.from(host.children);
for(const node of original){
if(node.tagName==='SCRIPT')continue;
if(node.matches('nav.nav')){node.remove();continue;}
if(node.matches('div.grid')&&node.querySelector('button[onclick="scan()"]')){
for(const child of Array.from(node.children)){
 const target=child.textContent.includes('Network scan')?'wifi':'bluetooth';
 containers[target].append(child)}
 node.remove();continue}
let name='home';
if(['tv-power-panel','ir-presets','infrared'].includes(node.id))name='tv';
else if(node.id==='monitor-power'||node.querySelector('#monitor-file'))name='monitors';
else if(['ble-advertising','bluetooth'].includes(node.id))name='bluetooth';
else if(node.id==='wifi'||node.querySelector('#signal-wifi'))name='wifi';
else if(node.id==='badble')name='badble';
else if(node.id==='privacy')name='settings';
else if(node.matches('p')&&node.textContent.includes('Access point:'))name='wifi';
containers[name].append(node)
}
const prev=document.createElement('button'),next=document.createElement('button'),indicator=document.createElement('div');
prev.type='button';next.type='button';prev.textContent='◀ PREVIOUS';next.textContent='NEXT ▶';prev.setAttribute('aria-label','Previous page');next.setAttribute('aria-label','Next page');
indicator.className='skb-page-indicator';indicator.setAttribute('aria-live','polite');
footer.append(prev,indicator,next);host.prepend(tabs);host.append(deck,footer);
let current=0;
function show(id){const n=typeof id==='number'?Math.max(0,Math.min(specs.length-1,id)):specs.findIndex(x=>x[0]===id);if(n<0)return;current=n;
for(let i=0;i<specs.length;i++){const key=specs[i][0];const on=i===n;containers[key].classList.toggle('skb-active',on);containers[key].setAttribute('aria-hidden',String(!on));tabs.children[i].setAttribute('aria-selected',String(on))}
prev.disabled=n===0;next.disabled=n===specs.length-1;indicator.textContent=specs[n][1]+' · '+(n+1)+' / '+specs.length;
const hint=document.createElement('small');hint.textContent='Swipe to change pages';indicator.append(hint);
containers[specs[n][0]].scrollTop=0;tabs.children[n].scrollIntoView({block:'nearest',inline:'nearest'});}
prev.addEventListener('click',()=>show(current-1));next.addEventListener('click',()=>show(current+1));
let startX=0,startY=0,swipe=false;
deck.addEventListener('touchstart',e=>{if(e.touches.length!==1||e.target.closest('input,select,textarea')){swipe=false;return}swipe=true;startX=e.touches[0].clientX;startY=e.touches[0].clientY},{passive:true});
deck.addEventListener('touchend',e=>{if(!swipe||!e.changedTouches.length)return;swipe=false;const dx=e.changedTouches[0].clientX-startX,dy=e.changedTouches[0].clientY-startY;
if(Math.abs(dx)>=65&&Math.abs(dx)>Math.abs(dy)*1.35)show(current+(dx<0?1:-1))},{passive:true});
const oldHash=location.hash.replace('#','');const initial=oldHash==='wifi'?'wifi':oldHash==='bluetooth'?'bluetooth':oldHash==='infrared'?'tv':oldHash==='privacy'?'settings':oldHash==='device'?'home':oldHash.startsWith('skb-page-')?oldHash.slice(9):'home';show(initial);
})();
</script><script>
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
  skbBadBleTick();
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
