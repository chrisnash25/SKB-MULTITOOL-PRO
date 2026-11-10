#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

// Shared, explicitly non-transmitting diagnostics for Wi-Fi and USB transports.
// A selected mode is not evidence of a connected chip.
namespace MamboRadio {
inline bool supportedMode(const String& mode) {
  return mode=="none" || mode=="cc1101" || mode=="nrf24";
}
inline void fillDiagnostics(JsonDocument& doc, const String& mode) {
  doc["ok"]=true;
  doc["product"]="Mambo OS V3";
  doc["selected_mode"]=mode;
  doc["single_module"]=true;
  doc["module_present"]=false;
  doc["hardware_detected"]=false;
  doc["hardware_detection"]="not_probed";
#ifdef MAMBO_RADIO_HW_VERIFIED
  doc["probe_available"]=true;
  doc["reason"]="Use explicit read-only SPI probe to check selected radio";
#else
  doc["probe_available"]=false;
  doc["reason"]="GPIO output assignments and supply not electrically validated";
#endif
  doc["physical_monitoring"]="unavailable";
  doc["transmission_enabled"]=false;
  doc["swap_requires_power_off"]=true;
  doc["connection"]="software_only";
  doc["cc1101"]["ready"]=false;
  doc["nrf24"]["ready"]=false;
  doc["cc1101"]["capabilities"]="SPI identification scaffold; no configured RX/RSSI sampling";
  doc["nrf24"]["capabilities"]="SPI identification scaffold; no configured RPD channel sampling";
  doc["sample_format"]="offline JSON array of frequency/channel and observed power";
  doc["note"]="Selection is a saved preference, not proof of module presence";
}
} // namespace MamboRadio
