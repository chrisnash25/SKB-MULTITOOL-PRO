#pragma once
// SKB USB-OTG serial RPC v1. Device commands use newline-delimited JSON at 115200 8N1.
// Retains Wi-Fi AP as a fallback; Android controls USB without joining device Wi-Fi.
// This file is included by main.cpp after global state declarations.
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <vector>
static File skbUsbUpload;
static String skbUsbUploadName;
static size_t skbUsbUploadBytes=0;
inline bool skbUsbValidIRName(const String& name){
 if(name.length()<1||name.length()>55||!name.endsWith(".ir")||name.indexOf("..")>=0)return false;
 for(size_t i=0;i<name.length();++i){char c=name[i];if(!isalnum((unsigned char)c)&&c!='_'&&c!='-'&&c!='.')return false;}return true;
}
inline void skbUsbReplyError(JsonDocument& res,const String& code){res["ok"]=false;res["error"]=code;}
inline bool skbUsbHandle(JsonDocument& req,JsonDocument& res){
 String cmd=req["cmd"] | "";
 if(!cmd.startsWith("usb_"))return false;
 res["id"]=req["id"] | 0;
 res["ok"]=true;
 if(cmd=="usb_hello"){res["protocol"]="skb-usb-json-v1";res["device"]="SKB MULTITOOL PRO";res["transport"]="USB CH9102 serial";res["baud"]=115200;res["ir_storage"]=filesystemReady;}
 else if(cmd=="usb_status"){
  res["device"]="SKB MULTITOOL PRO";res["uptime_ms"]=millis();res["free_heap"]=ESP.getFreeHeap();
  res["battery_voltage"]=M5.Power.getBatteryVoltage();res["wifi_ap_ip"]=WiFi.softAPIP().toString();
  res["wifi_sta_connected"]=(WiFi.status()==WL_CONNECTED);res["pathshield_enabled"]=pathShieldEnabled;
  res["usb_ready"]=true;
 }
 else if(cmd=="usb_ir_library"){
  JsonArray files=res["files"].to<JsonArray>();
  if(!filesystemReady){skbUsbReplyError(res,"storage_unavailable");return true;}
  File root=LittleFS.open("/ir");
  if(root){File f=root.openNextFile();while(f){String path=f.name();if(!f.isDirectory()&&path.endsWith(".ir")){JsonObject item=files.add<JsonObject>();item["name"]=path.substring(path.lastIndexOf('/')+1);item["bytes"]=f.size();}f=root.openNextFile();}}
 }
 else if(cmd=="usb_ir_keys" || cmd=="usb_ir_send"){
  String name=req["file"] | "";String key=req["key"] | "";
  if(!skbUsbValidIRName(name)){skbUsbReplyError(res,"invalid_file");return true;}
  if(!filesystemReady){skbUsbReplyError(res,"storage_unavailable");return true;}
  File f=LittleFS.open("/ir/"+name,"r");if(!f){skbUsbReplyError(res,"file_not_found");return true;}
  if(f.size()>32000){f.close();skbUsbReplyError(res,"file_too_large");return true;}
  String src=f.readString();f.close();std::vector<SKBIR::Signal> signals;String error;
  if(!SKBIR::parse(src,signals,error)){skbUsbReplyError(res,error);return true;}
  if(cmd=="usb_ir_keys"){JsonArray keys=res["keys"].to<JsonArray>();for(const auto &signal:signals){JsonObject item=keys.add<JsonObject>();item["name"]=signal.name;item["protocol"]=signal.protocol;}}
  else {
   bool found=false;for(const auto &signal:signals){if(signal.name==key){found=true;if(!SKBIR::send(signal,error))skbUsbReplyError(res,error);else{res["sent"]=key;res["note"]="IR transmitted; target response unverified";}break;}}
   if(!found)skbUsbReplyError(res,"key_not_found");
  }
 }
 else if(cmd=="usb_ir_tv_power"){
  String brand=req["brand"] | "";
  if(brand=="Samsung")IrSender.sendSamsung(0x07,0x02,0);
  else if(brand=="LG")IrSender.sendNEC(0x04,0x08,0);
  else skbUsbReplyError(res,"unverified_tv_brand");
  if(res["ok"].as<bool>())res["note"]="Power toggle sent (state unverified)";
 }
 else if(cmd=="usb_ir_nec"){
  int a=req["address"] | -1,c=req["command"] | -1;
  if(a<0||a>65535||c<0||c>255)skbUsbReplyError(res,"invalid_nec_values");
  else{IrSender.sendNEC((uint16_t)a,(uint8_t)c,0);res["note"]="NEC signal sent";}
 }
 else if(cmd=="usb_wifi_scan"){
  if(wifiScanning)skbUsbReplyError(res,"scan_running");
  else{WiFi.scanDelete();WiFi.scanNetworks(true,true);wifiScanning=true;scanStarted=millis();res["state"]="scanning";}
 }
 else if(cmd=="usb_wifi_results"){
  int n=WiFi.scanComplete();if(n==WIFI_SCAN_RUNNING){res["state"]="scanning";if(millis()-scanStarted>15000){WiFi.scanDelete();wifiScanning=false;skbUsbReplyError(res,"scan_timeout");}}
  else if(n<0){res["state"]="idle";wifiScanning=false;}
  else{res["state"]="complete";wifiScanning=false;JsonArray arr=res["networks"].to<JsonArray>();for(int i=0;i<n&&i<64;i++){JsonObject a=arr.add<JsonObject>();a["ssid"]=WiFi.SSID(i);a["rssi"]=WiFi.RSSI(i);a["channel"]=WiFi.channel(i);a["bssid"]=WiFi.BSSIDstr(i);}WiFi.scanDelete();}
 }
 else if(cmd=="usb_ble_scan"){
  if(!bleScanner)skbUsbReplyError(res,"ble_not_ready");
  else{
   BLEScanResults scan=bleScanner->start(3,false);
   JsonArray devices=res["devices"].to<JsonArray>();int n=scan.getCount();unsigned candidates=0;bool tracked=false;
   for(int i=0;i<n&&i<40;i++){
    BLEAdvertisedDevice d=scan.getDevice(i);JsonObject item=devices.add<JsonObject>();String addr=String(d.getAddress().toString().c_str());
    item["address"]=addr;item["rssi"]=d.getRSSI();if(d.haveName())item["name"]=String(d.getName().c_str());
    if(trackedBleAddress.length()&&addr.equalsIgnoreCase(trackedBleAddress)){tracked=true;trackedBleRssi=d.getRSSI();trackedBleLastSeenMs=millis();}
    if(pathShieldEnabled&&d.haveManufacturerData()){item["tracker_candidate"]="manufacturer data present; not definitive";candidates++;}
   }
   trackedBleSeen=tracked;if(pathShieldEnabled){trackerCandidatesLastScan=candidates;trackerAlert=candidates?"BLE candidates require manual review":"No BLE tracker candidates observed";}
   res["tracker_candidates"]=candidates;bleScanner->clearResults();
  }
 }
 else if(cmd=="usb_ble_advertise"){
  if(!bleAdvertiser)skbUsbReplyError(res,"ble_advertiser_unavailable");
  else if(req["enabled"].is<bool>()){bool enabled=req["enabled"].as<bool>();if(enabled)bleAdvertiser->start();else bleAdvertiser->stop();bleAdvertisingEnabled=enabled;}
  res["enabled"]=bleAdvertisingEnabled;res["name"]="SKB-MULTITOOL-PRO";
 }
 else if(cmd=="usb_ble_tracking"){
  if(req["address"].is<const char*>()){
   String addr=req["address"].as<String>();addr.trim();if(addr.length()&&(addr.length()!=17||addr.charAt(2)!=':'||addr.charAt(5)!=':'||addr.charAt(8)!=':'||addr.charAt(11)!=':'||addr.charAt(14)!=':'))skbUsbReplyError(res,"invalid_ble_address");
   else {trackedBleAddress=addr;trackedBleSeen=false;skbPrefs.putString("ble_target",addr);}
  }
  res["address"]=trackedBleAddress;res["seen_this_scan"]=trackedBleSeen;res["rssi"]=trackedBleRssi;
 }
 else if(cmd=="usb_pathshield"){
  if(req["enabled"].is<bool>()){pathShieldEnabled=req["enabled"].as<bool>();skbPrefs.putBool("pathshield",pathShieldEnabled);pathShieldEnabledSince=millis();}
  res["enabled"]=pathShieldEnabled;res["candidate_count"]=trackerCandidatesLastScan;res["alert"]=trackerAlert;
  res["note"]="Candidate counts are informational; not confirmed trackers";
 }
 else if(cmd=="usb_ir_import_begin"){
  String name=req["file"] | "";
  if(!skbUsbValidIRName(name)||!filesystemReady){skbUsbReplyError(res,"invalid_filename_or_storage");}
  else{if(skbUsbUpload)skbUsbUpload.close();LittleFS.remove("/ir/.usb_upload.tmp");
   skbUsbUpload=LittleFS.open("/ir/.usb_upload.tmp","w");skbUsbUploadName=name;skbUsbUploadBytes=0;
   if(!skbUsbUpload)skbUsbReplyError(res,"cannot_open_upload");}
 }
 else if(cmd=="usb_ir_import_chunk"){
  String data=req["data"] | "";
  if(!skbUsbUpload||!data.length()||data.length()>220||skbUsbUploadBytes+data.length()>32000)skbUsbReplyError(res,"invalid_upload_chunk");
  else{size_t n=skbUsbUpload.print(data);skbUsbUploadBytes+=n;if(n!=data.length())skbUsbReplyError(res,"upload_write_failed");res["bytes"]=skbUsbUploadBytes;}
 }
 else if(cmd=="usb_ir_import_end"){
  if(!skbUsbUpload){skbUsbReplyError(res,"no_upload");}
  else{
   skbUsbUpload.close();File f=LittleFS.open("/ir/.usb_upload.tmp","r");
   if(!f)skbUsbReplyError(res,"upload_missing");
   else{String body=f.readString();f.close();std::vector<SKBIR::Signal> signals;String err;
    if(!SKBIR::parse(body,signals,err)){skbUsbReplyError(res,err);}
    else if(!LittleFS.rename("/ir/.usb_upload.tmp","/ir/"+skbUsbUploadName))skbUsbReplyError(res,"rename_failed");
    else{res["file"]=skbUsbUploadName;res["signals"]=signals.size();}
   }
   if(!res["ok"].as<bool>())LittleFS.remove("/ir/.usb_upload.tmp");
  }
 }
 else if(cmd=="usb_ir_import_abort"){if(skbUsbUpload)skbUsbUpload.close();LittleFS.remove("/ir/.usb_upload.tmp");}
 else skbUsbReplyError(res,"unknown_usb_command");
 return true;
}
