#include <cassert>
#include <iostream>
#include <WiFi.h>
#include "wifi/WifiDiagnostics.h"
#include "wifi/ChannelAnalysis.h"
uint32_t testNow=100;
TestWiFi WiFi;
int main() {
  using skb::overlapScore;
  assert(overlapScore(1,-50,1)==250);
  assert(overlapScore(1,-50,6)==0);
  assert(overlapScore(1,-50,3)==150);
  assert(overlapScore(0,-50,1)==0);
  assert(overlapScore(1,-150,1)==5);
  Preferences prefs;
  WifiDiagnostics wifi;wifi.begin(prefs);
  JsonDocument req,out;
  auto command=[&](const char* cmd){out.clear();return wifi.command(cmd,req.as<JsonVariantConst>(),out);};
  assert(command("status")==0);
  assert(command("wifi.results")==200 && out["state"]=="idle" && !out["has_results"].as<bool>());
  assert(command("wifi.scan")==202 && WiFi.passive && WiFi.dwell==120);
  assert(command("wifi.scan")==409 && WiFi.starts==1);
  wifi.tick(); assert(command("wifi.results")==200 && out["state"]=="scanning");
  WiFi.aps={{"AA:BB:CC:DD:EE:01","Lab",-50,1},{"AA:BB:CC:DD:EE:02","",-80,6}};
  WiFi.complete=2;testNow=500;wifi.tick();
  assert(command("wifi.results")==200 && out["networks"].size()==2 && out["generation"]==1);
  // Reading results repeatedly preserves the last completed snapshot.
  assert(command("wifi.results")==200 && out["state"]=="complete" && out["networks"].size()==2);
  assert(command("wifi.channels")==200 && out["channels"].size()==11 && out["suggested_channel"]==11);
  assert(out["channels"][0]["overlap_score"]==250);
  assert(command("wifi.signals")==200 && out["signals"].size()==2 && !out["live"].as<bool>());
  assert(command("wifi.devices")==200 && out["access_points"].size()==2 && out["local_ap_clients"].size()==1);
  WiFi.complete=WIFI_SCAN_RUNNING;assert(command("wifi.scan")==202);
  WiFi.aps.clear();WiFi.complete=0;testNow=600;wifi.tick();
  assert(command("wifi.results")==200 && out["networks"].size()==0 && out["generation"]==2);
  assert(command("wifi.devices")==200 && !out["access_points"][0]["seen_last_scan"].as<bool>() && out["access_points"][0]["age_ms"]==100);
  req["label"]="Lab Stick";req["scan_dwell_ms"]=250;
  assert(command("config.set")==200 && out["label"]=="Lab Stick" && prefs.storedDwell==250);
  req["scan_dwell_ms"]=99;assert(command("config.set")==400 && prefs.storedDwell==250);
  req["scan_dwell_ms"]=250;req["label"]="";assert(command("config.set")==400);
  req["label"]="\n";assert(command("config.set")==400);
  req["label"]="Lab";prefs.fail=true;assert(command("config.set")==500);prefs.fail=false;
  WiFi.complete=WIFI_SCAN_RUNNING;assert(command("wifi.scan")==202);
  assert(command("config.set")==409);
  testNow+=20001;wifi.tick();assert(command("wifi.results")==200 && out["state"]=="timeout" && !out["ok"].as<bool>());
  WiFi.startResult=WIFI_SCAN_FAILED;assert(command("wifi.scan")==503);
  WiFi.startResult=WIFI_SCAN_RUNNING;assert(command("wifi.scan")==202);WiFi.complete=WIFI_SCAN_FAILED;wifi.tick();
  assert(command("wifi.results")==200 && out["state"]=="failed");
  // Bound scan snapshots and history even when the driver reports many APs.
  WiFi.complete=WIFI_SCAN_RUNNING;assert(command("wifi.scan")==202);
  std::vector<std::string> addresses;
  for (int i=0;i<65;++i) addresses.push_back("ap-"+std::to_string(i));
  WiFi.aps.clear();
  for (int i=0;i<65;++i) WiFi.aps.push_back({addresses[i].c_str(),"Lab",-60,6});
  WiFi.complete=65;testNow+=500;wifi.tick();
  assert(command("wifi.results")==200 && out["truncated"].as<bool>() && out["networks"].size()==64);
  assert(command("wifi.devices")==200 && out["access_points"].size()==64);
  WifiDiagnostics restored;restored.begin(prefs);
  out.clear();assert(restored.command("config.get",req.as<JsonVariantConst>(),out)==200 && out["label"]=="Lab Stick");
  std::cout << "Wi-Fi diagnostics: lifecycle, passive scan, caching, channels, monitoring, settings and failures passed\n";
}
