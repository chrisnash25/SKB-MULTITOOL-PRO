#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include "mambo_radio_probe.h"

// Receive-only nRF24L01+ RPD sampler. No TX FIFO writes, TX modes,
// payload decoding, or packet reconstruction. RPD is a binary threshold
// (~-64 dBm), NOT a calibrated RSSI measurement.
// Available only with MAMBO_RADIO_HW_VERIFIED and explicit pin definitions.
namespace MamboRadio {
inline void observeChannels(JsonDocument& out, const String& mode, int firstChannel, int count) {
  out["ok"]=false;
  out["mode"]=mode;
  out["transmission_enabled"]=false;
  out["measurement"]="binary_RPD";
  if(mode!="nrf24") {
    out["error"]="nrf24_only";
    return;
  }
  if(firstChannel<0 || firstChannel>125 || count<1 || count>16 ||
     firstChannel+count>126) {
    out["error"]="invalid_channel_range";
    return;
  }
#ifndef MAMBO_RADIO_HW_VERIFIED
  out["error"]="hardware_monitor_disabled";
  out["reason"]="Verified GPIO and 3.3V power profile required";
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
  if(!valid(p)) { out["error"]="invalid_gpio_map";return; }
  pinMode(p.csn,OUTPUT); digitalWrite(p.csn,HIGH);
  pinMode(p.control,OUTPUT); digitalWrite(p.control,LOW); // CE
  SPI.begin(p.sck,p.miso,p.mosi,p.csn);
  SPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE0));
  auto writeRegister=[&](uint8_t reg,uint8_t value) {
    digitalWrite(p.csn,LOW);
    SPI.transfer((uint8_t)(0x20|(reg&0x1f)));
    SPI.transfer(value);
    digitalWrite(p.csn,HIGH);
  };
  auto readRegister=[&](uint8_t reg)->uint8_t {
    digitalWrite(p.csn,LOW);
    SPI.transfer((uint8_t)(reg&0x1f));
    uint8_t value=SPI.transfer(0xff);
    digitalWrite(p.csn,HIGH);
    return value;
  };
  // Receiver only: disable auto-ack, do not load payloads or send a packet.
  writeRegister(0x00,0x00); // CONFIG power down while changing settings
  writeRegister(0x01,0x00); // EN_AA no acknowledgements
  writeRegister(0x02,0x01); // EN_RXADDR pipe zero
  writeRegister(0x11,0x01); // RX_PW_P0 minimal width
  writeRegister(0x00,0x03); // CONFIG PWR_UP | PRIM_RX
  delay(2);                // oscillator start
  JsonArray values=out["observations"].to<JsonArray>();
  for(int ch=firstChannel;ch<firstChannel+count;ch++) {
    digitalWrite(p.control,LOW);
    writeRegister(0x05,(uint8_t)ch); // RF_CH
    digitalWrite(p.control,HIGH);
    delayMicroseconds(330);          // RX settle + RPD accumulation
    const bool seen=(readRegister(0x09)&0x01)!=0; // RPD
    digitalWrite(p.control,LOW);
    JsonObject row=values.add<JsonObject>();
    row["channel"]=ch;
    row["frequency_mhz"]=2400+ch;
    row["detected"]=seen;
  }
  writeRegister(0x00,0x00); // power down radio receiver
  SPI.endTransaction();
  SPI.end();
  out["ok"]=true;
  out["samples"]=count;
  out["note"]="RPD is a threshold event, not dBm RSSI or decoded traffic";
#endif
}
} // namespace MamboRadio
