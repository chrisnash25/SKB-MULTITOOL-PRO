#pragma once
#include <cstdint>
constexpr int ESP_OK=0;
struct wifi_country_t {uint8_t schan=1,nchan=11;};
struct TestSta {uint8_t mac[6]={1,2,3,4,5,6};int8_t rssi=-50;};
struct wifi_sta_list_t {int num=1;TestSta sta[1];};
inline int esp_wifi_get_country(wifi_country_t*) {return ESP_OK;}
inline int esp_wifi_ap_get_sta_list(wifi_sta_list_t*) {return ESP_OK;}

inline int esp_wifi_scan_stop() {return ESP_OK;}
