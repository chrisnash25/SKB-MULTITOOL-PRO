#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include "mambo_radio_probe.h"

// Minimal receive-only CC1101 RSSI observations for 26 MHz crystal CC1101
// modules configured for the 433 MHz band. Never strobes STX.
// The RF/power/antenna configuration still requires physical verification.
namespace MamboRadio {
inline void observeCc1101(JsonDocument& out,const String& mode,int startKhz=433050,int count=8,int stepKhz=100) {
  out["ok"]=false;
  out["mode"]=mode;
  out["transmission_enabled"]=false;
  out["measurement"]="RSSI_dBm_estimate";
  if(mode!="cc1101") {out["error"]="cc1101_only";return;}
  if(count<1 || count>16 || stepKhz<25 || stepKhz>500 || startKhz<433050 ||
     startKhz+(count-1)*stepKhz>434790) {out["error"]="invalid_433mhz_scan_range";return;}
#ifndef MAMBO_RADIO_HW_VERIFIED
  out["error"]="hardware_monitor_disabled";
  out["reason"]="Verified GPIO/power profile required";
#else
  Pins p;
  p.sck=MAMBO_RADIO_PIN_SCK;
  p.miso=MAMBO_RADIO_PIN_MISO;
  p.mosi=MAMBO_RADIO_PIN_MOSI;
  p.csn=MAMBO_RADIO_PIN_CSN;
  p.control=MAMBO_RADIO_PIN_CONTROL;
#ifdef MAMBO_RADIO_PIN_IRQ
  p.irq=MAMBO_RADIO_PIN_IRQ;
#endif
  if(!valid(p)){out["error"]="invalid_gpio_map";return;}
  pinMode(p.csn,OUTPUT);digitalWrite(p.csn,HIGH);
  pinMode(p.control,INPUT); // CC1101 GDO0, never driven
  SPI.begin(p.sck,p.miso,p.mosi,p.csn);
  SPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE0));
  auto writeReg=[&](uint8_t reg,uint8_t val) {
    digitalWrite(p.csn,LOW);SPI.transfer(reg);SPI.transfer(val);
    digitalWrite(p.csn,HIGH);
  };
  auto strobe=[&](uint8_t command) {
    digitalWrite(p.csn,LOW);SPI.transfer(command);digitalWrite(p.csn,HIGH);
  };
  auto readStatus=[&](uint8_t reg)->uint8_t {
    digitalWrite(p.csn,LOW);SPI.transfer((uint8_t)(reg|0xc0));
    uint8_t x=SPI.transfer(0xff);digitalWrite(p.csn,HIGH);
    return x;
  };
  const uint8_t version=readStatus(0x31);
  out["version"]=version;
  if(version==0 || version==0xff) {
    out["error"]="cc1101_not_detected";
    SPI.endTransaction();SPI.end();return;
  }
  // Limited 433.05–434.79 MHz receive-only scan (26 MHz CC1101 crystal).
  // RSSI offset is approximate and has not been calibrated for this board.
  writeReg(0x18,0x18); // MCSM0, auto-calibrate when leaving IDLE
  JsonArray bins=out["observations"].to<JsonArray>();
  int validSamples=0;
  for(int i=0;i<count;i++) {
    const int frequencyKhz=startKhz+i*stepKhz;
    const uint32_t frequencyWord=(uint32_t)(((uint64_t)frequencyKhz*1000ULL*65536ULL)/26000000ULL);
    strobe(0x36); // SIDLE
    writeReg(0x0d,(uint8_t)((frequencyWord>>16)&0xff));
    writeReg(0x0e,(uint8_t)((frequencyWord>>8)&0xff));
    writeReg(0x0f,(uint8_t)(frequencyWord&0xff));
    strobe(0x34); // SRX: never STX
    delay(4);
    const uint8_t marcstate=readStatus(0x35)&0x1f;
    const uint8_t raw=readStatus(0x34);
    strobe(0x36);
    JsonObject row=bins.add<JsonObject>();
    row["frequency_mhz"]=frequencyKhz/1000.0f;
    if(marcstate==0x0d) {
      const int signedRssi=raw>=128?(int)raw-256:(int)raw;
      row["power_dbm"]=signedRssi/2.0f-74.0f;
      validSamples++;
    } else row["status"]="receiver_not_in_rx";
  }
  SPI.endTransaction();SPI.end();
  out["ok"]=validSamples>0;
  out["samples"]=validSamples;
  out["requested"]=count;
  if(!validSamples)out["error"]="receiver_not_in_rx";
  out["note"]="Approximate RSSI values; only 433 MHz band; no packet decoding";

#endif
}
} // namespace MamboRadio
