#pragma once
#include <Arduino.h>
#include <SPI.h>

// Interchangeable single-module interface for the M5StickC Plus2.
// This module performs read-only presence checks; no RF transmissions.
namespace MamboRadio {
enum class Module : uint8_t { None, CC1101, NRF24 };
struct Pins {
  int sck=-1, miso=-1, mosi=-1, csn=-1, control=-1, irq=-1;
};
struct Status {
  Module selected=Module::None;
  bool configured=false;
  bool detected=false;
  uint8_t registerValue=0;
};
inline bool valid(const Pins& p) {
  const int required[]={p.sck,p.miso,p.mosi,p.csn,p.control};
  for(int i=0;i<5;i++) {
    if(required[i]<0 || required[i]>39) return false;
    for(int j=0;j<i;j++) if(required[i]==required[j]) return false;
  }
  if(p.irq>=0) {
    if(p.irq>39) return false;
    for(int i=0;i<5;i++) if(p.irq==required[i]) return false;
  }
  return true;
}
class Monitor {
  Status state;
public:
  Status begin(Module module,const Pins& p) {
    state=Status{};
    state.selected=module;
    if(module==Module::None || !valid(p)) return state;
    // A radio must be physically swapped only with the board powered off.
    pinMode(p.csn,OUTPUT);
    digitalWrite(p.csn,HIGH);
    if(module==Module::NRF24) {
      pinMode(p.control,OUTPUT); // CE
      digitalWrite(p.control,LOW);
    } else {
      pinMode(p.control,INPUT); // GDO0
    }
    if(p.irq>=0) pinMode(p.irq,INPUT);
    SPI.begin(p.sck,p.miso,p.mosi,p.csn);
    SPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE0));
    digitalWrite(p.csn,LOW);
    if(module==Module::CC1101) {
      // CC1101 VERSION status register: read + burst bits, address 0x31.
      SPI.transfer(0xF1);
      state.registerValue=SPI.transfer(0xFF);
    } else {
      // nRF24 NOP returns STATUS without modifying registers.
      state.registerValue=SPI.transfer(0xFF);
    }
    digitalWrite(p.csn,HIGH);
    SPI.endTransaction();
    state.configured=true;
    // SPI response alone is indicative, not proof of a healthy module.
    state.detected=state.registerValue!=0x00 && state.registerValue!=0xFF;
    return state;
  }
  Status status() const { return state; }
};
} // namespace MamboRadio
