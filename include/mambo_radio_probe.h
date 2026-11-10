#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "mambo_radio_lab.h"

// Compile-time hardware gate: intentionally disabled in the default firmware.
// Enable only in a variant whose wiring, output GPIOs and 3.3 V budget have
// been verified against the *actual* M5StickC Plus2 + interchangeable socket.
#if defined(MAMBO_RADIO_HW_VERIFIED)
  #if !defined(MAMBO_RADIO_PIN_SCK) || !defined(MAMBO_RADIO_PIN_MISO) || \
      !defined(MAMBO_RADIO_PIN_MOSI) || !defined(MAMBO_RADIO_PIN_CSN) || \
      !defined(MAMBO_RADIO_PIN_CONTROL)
    #error "Verified radio builds require SCK, MISO, MOSI, CSN and CONTROL pins"
  #endif
#endif

namespace MamboRadio {
inline void probeReadOnly(JsonDocument& out, const String& mode) {
  out["ok"]=false;
  out["selected_mode"]=mode;
  out["transmission_enabled"]=false;
  out["probe_type"]="SPI register/status read only";
  if(mode!="cc1101" && mode!="nrf24") {
    out["error"]="select_module_first";
    return;
  }
#ifndef MAMBO_RADIO_HW_VERIFIED
  out["error"]="hardware_probe_disabled";
  out["reason"]="Custom validated GPIO/power profile not configured";
#else
  Pins pins;
  pins.sck=MAMBO_RADIO_PIN_SCK;
  pins.miso=MAMBO_RADIO_PIN_MISO;
  pins.mosi=MAMBO_RADIO_PIN_MOSI;
  pins.csn=MAMBO_RADIO_PIN_CSN;
  pins.control=MAMBO_RADIO_PIN_CONTROL;
#ifdef MAMBO_RADIO_PIN_IRQ
  pins.irq=MAMBO_RADIO_PIN_IRQ;
#endif
  if(!valid(pins)) {
    out["error"]="invalid_gpio_map";
    return;
  }
  Monitor monitor;
  const Status status=monitor.begin(mode=="cc1101"?Module::CC1101:Module::NRF24,pins);
  out["ok"]=status.configured;
  out["hardware_detected"]=status.detected;
  out["register_value"]=status.registerValue;
  out["configured"]=status.configured;
  out["note"]="SPI response is indicative, not proof of module health";
#endif
}
} // namespace MamboRadio
