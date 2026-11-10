#pragma once
// SKB passive radio/system diagnostics. Safe normal Wi-Fi scanning only.
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <LittleFS.h>
inline void skbDiagnostics(JsonDocument& out){
 out["ok"]=true;out["device"]="M5StickC Plus2";
 out["uptime_ms"]=millis();out["cpu_mhz"]=ESP.getCpuFreqMHz();
 out["heap_free_bytes"]=ESP.getFreeHeap();
 out["heap_total_bytes"]=ESP.getHeapSize();
 out["flash_bytes"]=ESP.getFlashChipSize();
 out["battery_voltage"]=M5.Power.getBatteryVoltage();
 out["wifi_ap_ip"]=WiFi.softAPIP().toString();
 out["wifi_ap_clients"]=WiFi.softAPgetStationNum();
 out["filesystem_ready"]=filesystemReady;
 if(filesystemReady){
  out["littlefs_total_bytes"]=LittleFS.totalBytes();
  out["littlefs_used_bytes"]=LittleFS.usedBytes();
 }
 out["ble_keyboard_connected"]=skbKeyboard.isConnected();
 out["usb_transport"]="CH9102 UART 115200";
 out["native_usb_hid"]=false;
}
inline void skbChannelSurvey(JsonDocument& out){
 out["ok"]=true;out["passive"]=true;out["source"]="last_or_current_scan";
 JsonArray channels=out["channels"].to<JsonArray>();
 int counts[14]={0};int strongest[14];for(int i=0;i<14;i++)strongest[i]=-127;
 int n=WiFi.scanComplete();
 if(n>=0){
  for(int i=0;i<n;i++){
   int ch=WiFi.channel(i);
   if(ch>=1 && ch<=13){counts[ch]++;int r=WiFi.RSSI(i);if(r>strongest[ch])strongest[ch]=r;}
  }
 }
 for(int ch=1;ch<=13;ch++){
  JsonObject row=channels.add<JsonObject>();
  row["channel"]=ch;row["ap_count"]=counts[ch];
  if(counts[ch])row["strongest_rssi_dbm"]=strongest[ch];
 }
 if(n<0)out["note"]="Run a Wi-Fi scan first; no completed results currently available.";
 else out["note"]="Survey summarizes discovered AP beacons; not raw packet capture.";
}
