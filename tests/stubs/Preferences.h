#pragma once
#include "Arduino.h"
class Preferences {
 public:
  String storedLabel = "SKB MULTITOOL PRO";
  uint16_t storedDwell=120;
  bool fail=false;
  String getString(const char*,const char*) {return storedLabel;}
  uint16_t getUShort(const char*,uint16_t) {return storedDwell;}
  size_t putString(const char*,const String& label) {if(fail)return 0;storedLabel=label;return label.length();}
  size_t putUShort(const char*,uint16_t dwell) {if(fail)return 0;storedDwell=dwell;return 2;}
};
