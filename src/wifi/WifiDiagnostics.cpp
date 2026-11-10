#include "WifiDiagnostics.h"
#include "ChannelAnalysis.h"
#include <WiFi.h>
#include <esp_wifi.h>

void WifiDiagnostics::begin(Preferences& prefs) {
  prefs_ = &prefs;
  label_ = prefs.getString("device_label", "SKB MULTITOOL PRO");
  dwell_ = prefs.getUShort("scan_dwell", 120);
  if (dwell_ < 100 || dwell_ > 500) dwell_ = 120;
}

void WifiDiagnostics::tick() {
  if (state_ != "scanning") return;
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING && millis()-started_ <= 20000) return;
  if (n < 0) {
    state_ = n == WIFI_SCAN_RUNNING ? "timeout" : "failed";
    esp_wifi_scan_stop();
    WiFi.scanDelete();
    return;
  }
  for (size_t i=0; i<count_; ++i) observations_[i].seen = false;
  truncated_ = n > 64;
  const uint32_t now = millis();
  for (int i=0; i<n && i<64; ++i) {
    String bssid = WiFi.BSSIDstr(i);
    size_t slot = 0;
    while (slot<count_ && observations_[slot].bssid != bssid) ++slot;
    if (slot == count_) {
      if (count_ < 64) ++count_;
      else {
        // Evict only observations absent from this scan, oldest first.
        slot = 64;
        uint32_t oldestAge = 0;
        for (size_t j=0; j<count_; ++j) {
          uint32_t age = now-observations_[j].lastSeen;
          if (!observations_[j].seen && (slot == 64 || age > oldestAge)) {slot=j; oldestAge=age;}
        }
        if (slot == 64) {truncated_=true; continue;}
      }
      observations_[slot] = Observation();
      observations_[slot].bssid = bssid;
      observations_[slot].firstSeen = now;
    }
    auto& item = observations_[slot];
    item.ssid = WiFi.SSID(i); item.rssi = WiFi.RSSI(i);
    item.channel = WiFi.channel(i); item.encryption = static_cast<int>(WiFi.encryptionType(i));
    item.lastSeen = now; ++item.scansSeen; item.seen = true;
  }
  completed_ = now; ++generation_; state_ = "complete";
  WiFi.scanDelete();
}

void WifiDiagnostics::results(JsonDocument& doc, bool signals) {
  doc["ok"] = state_ != "failed" && state_ != "timeout";
  doc["state"] = state_; doc["passive"] = true;
  doc["generation"] = generation_; doc["completed_uptime_ms"] = completed_;
  doc["age_ms"] = generation_ ? millis()-completed_ : 0;
  doc["has_results"] = generation_ != 0; doc["truncated"] = truncated_;
  doc["source"] = "last_wifi_scan"; doc["live"] = false;
  JsonArray array = doc[signals ? "signals" : "networks"].to<JsonArray>();
  for (size_t i=0; i<count_; ++i) {
    const auto& item = observations_[i];
    if (!item.seen) continue;
    JsonObject ap = array.add<JsonObject>();
    ap["ssid"] = item.ssid; ap["bssid"] = item.bssid;
    ap["rssi"] = item.rssi; ap["channel"] = item.channel; ap["encryption"] = item.encryption;
  }
}

void WifiDiagnostics::channels(JsonDocument& doc) {
  doc["ok"] = true; doc["generation"] = generation_;
  doc["has_results"] = generation_ != 0; doc["state"] = state_;
  doc["age_ms"] = generation_ ? millis()-completed_ : 0;
  doc["truncated"] = truncated_;
  doc["metric"] = "relative_rssi_overlap_not_airtime";
  doc["band"] = "2.4GHz";
  wifi_country_t country{};
  int first=1, last=11;
  if (esp_wifi_get_country(&country) == ESP_OK) {first=country.schan; last=first+country.nchan-1;}
  JsonArray array = doc["channels"].to<JsonArray>();
  uint32_t bestScore = UINT32_MAX;
  int recommended = 0;
  for (int channel=first; channel<=last && channel<=14; ++channel) {
    uint32_t score=0; int aps=0;
    for (size_t i=0; i<count_; ++i) if (observations_[i].seen) {
      score += skb::overlapScore(observations_[i].channel, observations_[i].rssi, channel);
      if (observations_[i].channel == channel) ++aps;
    }
    JsonObject item = array.add<JsonObject>();
    item["channel"]=channel; item["ap_count"]=aps; item["overlap_score"]=score;
    if ((channel==1 || channel==6 || channel==11) && score<bestScore) {bestScore=score; recommended=channel;}
  }
  if (generation_) doc["suggested_channel"] = recommended;
}

void WifiDiagnostics::devices(JsonDocument& doc) {
  doc["ok"]=true; doc["generation"]=generation_;
  doc["note"]="AP observations and clients connected to this device only; absence is not proof of departure.";
  JsonArray aps=doc["access_points"].to<JsonArray>();
  for (size_t i=0; i<count_; ++i) {
    auto& item=observations_[i]; JsonObject ap=aps.add<JsonObject>();
    ap["bssid"]=item.bssid; ap["ssid"]=item.ssid; ap["rssi"]=item.rssi;
    ap["channel"]=item.channel; ap["seen_last_scan"]=item.seen;
    ap["first_seen_uptime_ms"]=item.firstSeen; ap["last_seen_uptime_ms"]=item.lastSeen;
    ap["age_ms"]=millis()-item.lastSeen; ap["scans_seen"]=item.scansSeen;
  }
  wifi_sta_list_t clients{};
  JsonArray list=doc["local_ap_clients"].to<JsonArray>();
  bool available=esp_wifi_ap_get_sta_list(&clients)==ESP_OK;
  doc["clients_available"]=available;
  if (available) for (int i=0; i<clients.num; ++i) {
    char mac[18]; const auto& sta=clients.sta[i];
    snprintf(mac,sizeof(mac),"%02X:%02X:%02X:%02X:%02X:%02X",sta.mac[0],sta.mac[1],sta.mac[2],sta.mac[3],sta.mac[4],sta.mac[5]);
    JsonObject client=list.add<JsonObject>(); client["address"]=mac; client["rssi"]=sta.rssi;
  }
}

int WifiDiagnostics::command(const String& cmd, JsonVariantConst request, JsonDocument& response) {
  if (cmd=="wifi.scan") {
    if (state_=="scanning") {response["ok"]=false; response["error"]="scan_running"; return 409;}
    WiFi.scanDelete();
    // Passive scan: no probe requests, no promiscuous payload capture.
    int result=WiFi.scanNetworks(true,true,true,dwell_);
    if (result==WIFI_SCAN_FAILED) {state_="failed"; response["ok"]=false; response["error"]="scan_start_failed"; return 503;}
    started_=millis(); state_="scanning"; response["ok"]=true; response["state"]=state_; return 202;
  }
  if (cmd=="wifi.results") {results(response); return 200;}
  if (cmd=="wifi.signals") {results(response,true); return 200;}
  if (cmd=="wifi.channels") {channels(response); return 200;}
  if (cmd=="wifi.devices") {devices(response); return 200;}
  if (cmd=="config.set") {
    if (!request["label"].is<String>() || !request["scan_dwell_ms"].is<int>()) {
      response["ok"]=false; response["error"]="label_and_scan_dwell_required"; return 400;
    }
    String label=request["label"].as<String>(); int dwell=request["scan_dwell_ms"].as<int>();
    bool valid=label.length()>0 && label.length()<=32 && dwell>=100 && dwell<=500;
    for (size_t i=0;i<label.length();++i) if (static_cast<uint8_t>(label[i])<32 || static_cast<uint8_t>(label[i])==127) valid=false;
    if (!valid) {response["ok"]=false; response["error"]="invalid_settings"; return 400;}
    if (state_=="scanning") {response["ok"]=false; response["error"]="scan_running"; return 409;}
    if (prefs_->putString("device_label",label)==0 || prefs_->putUShort("scan_dwell",dwell)==0) {
      response["ok"]=false; response["error"]="settings_write_failed"; return 500;
    }
    label_=label; dwell_=dwell;
  } else if (cmd!="config.get") return 0;
  response["ok"]=true; response["label"]=label_; response["scan_dwell_ms"]=dwell_;
  response["portal_ip"]=WiFi.softAPIP().toString(); return 200;
}
