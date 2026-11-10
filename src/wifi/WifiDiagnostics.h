#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

class WifiDiagnostics {
 public:
  void begin(Preferences& prefs);
  void tick();
  int command(const String& cmd, JsonVariantConst request, JsonDocument& response);
  const String& label() const { return label_; }
 private:
  struct Observation {
    String bssid, ssid;
    int rssi = -127, channel = 0, encryption = 0;
    uint32_t firstSeen = 0, lastSeen = 0, scansSeen = 0;
    bool seen = false;
  };
  Observation observations_[64];
  size_t count_ = 0;
  Preferences* prefs_ = nullptr;
  String label_ = "SKB MULTITOOL PRO";
  String state_ = "idle";
  uint32_t started_ = 0, completed_ = 0, generation_ = 0;
  uint16_t dwell_ = 120;
  bool truncated_ = false;
  void results(JsonDocument& doc, bool signals = false);
  void devices(JsonDocument& doc);
  void channels(JsonDocument& doc);
};
