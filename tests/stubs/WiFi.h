#pragma once
#include "Arduino.h"
#include <vector>
constexpr int WIFI_SCAN_RUNNING=-1, WIFI_SCAN_FAILED=-2;
struct TestAp {const char* bssid; const char* ssid; int rssi,channel;};
struct TestIP {String toString() const {return "192.168.4.1";}};
struct TestWiFi {
  std::vector<TestAp> aps;
  int complete=WIFI_SCAN_RUNNING, startResult=WIFI_SCAN_RUNNING, starts=0,deletes=0;
  bool passive=false;
  uint16_t dwell=0;
  void scanDelete(){++deletes;}
  int scanComplete(){return complete;}
  int scanNetworks(bool async,bool hidden,bool isPassive,uint16_t duration) {if(!async||!hidden)return WIFI_SCAN_FAILED;++starts;passive=isPassive;dwell=duration;return startResult;}
  String BSSIDstr(int i){return aps.at(i).bssid;}
  String SSID(int i){return aps.at(i).ssid;}
  int RSSI(int i){return aps.at(i).rssi;}
  int channel(int i){return aps.at(i).channel;}
  int encryptionType(int){return 3;}
  TestIP softAPIP(){return {};}
};
extern TestWiFi WiFi;
