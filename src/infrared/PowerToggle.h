#pragma once
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
class PowerToggle {
 public:
  explicit PowerToggle(uint16_t pin): sender(pin) {}
  void begin(){sender.begin();}
  bool sendSelected(const String& protocol, uint32_t address, uint32_t command, String& error){
    if(protocol=="NEC"){sender.sendNEC((uint64_t(address)<<16)|command,32);return true;}
    if(protocol=="Samsung32"){sender.sendSAMSUNG((uint64_t(address)<<16)|command,32);return true;}
    error="unsupported_protocol_or_encoding";return false;
  }
 private:
  IRsend sender;
};
