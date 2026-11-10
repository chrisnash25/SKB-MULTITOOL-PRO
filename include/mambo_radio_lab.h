#pragma once
#include <Arduino.h>
#include <SPI.h>

// Read-only radio diagnostics. GPIO assignments are intentionally explicit:
// do not enable until the external expansion wiring is verified.
namespace MamboRadio {
struct Pins { int sck=-1, miso=-1, mosi=-1, ccCs=-1, nrfCs=-1, nrfCe=-1; };
struct Status { bool configured=false, cc1101=false, nrf24=false; uint8_t ccVersion=0; uint8_t nrfStatus=0; };
inline bool valid(const Pins& p) {
  const int a[]={p.sck,p.miso,p.mosi,p.ccCs,p.nrfCs,p.nrfCe};
  for (int i=0;i<6;i++) { if(a[i]<0||a[i]>39) return false; for(int j=0;j<i;j++) if(a[i]==a[j]) return false; }
  return true;
}
class Monitor {
  Pins pins; Status state;
  uint8_t transferRegister(int cs,uint8_t address,uint8_t command) {
    SPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE0));
    digitalWrite(cs,LOW); SPI.transfer(address|command); uint8_t value=SPI.transfer(0xFF);
    digitalWrite(cs,HIGH); SPI.endTransaction(); return value;
  }
public:
  Status begin(const Pins& p) {
    state=Status{}; if(!valid(p)) return state;
    pins=p;
    pinMode(p.ccCs,OUTPUT);pinMode(p.nrfCs,OUTPUT);pinMode(p.nrfCe,OUTPUT);
    digitalWrite(p.ccCs,HIGH);digitalWrite(p.nrfCs,HIGH);digitalWrite(p.nrfCe,LOW);
    SPI.begin(p.sck,p.miso,p.mosi);
    state.configured=true;
    // CC1101 VERSION register 0x31; 0xC0 means burst/read.
    state.ccVersion=transferRegister(p.ccCs,0x31,0xC0);
    state.cc1101=(state.ccVersion!=0x00 && state.ccVersion!=0xFF);
    // nRF24 NOP command returns STATUS without altering radio state.
    SPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE0));
    digitalWrite(p.nrfCs,LOW);state.nrfStatus=SPI.transfer(0xFF);
    digitalWrite(p.nrfCs,HIGH);SPI.endTransaction();
    state.nrf24=(state.nrfStatus!=0x00 && state.nrfStatus!=0xFF);
    return state;
  }
  Status status() const { return state; }
};
} // namespace MamboRadio
