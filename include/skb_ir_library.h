#pragma once
// Flipper .ir text format: named decoded NEC records and raw pulse timings.
// Files are uploaded individually to LittleFS; the full IRDB cannot fit in internal flash.
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <IRremote.hpp>
#include <vector>
namespace SKBIR {
struct Signal { String name, protocol; uint32_t address=0,command=0,frequency=38000; std::vector<uint16_t> timings; };
inline bool parseNumber(String s,uint32_t &out) {
 s.trim();if(!s.length())return false;
 // Flipper IRDB stores little-endian hex bytes separated by spaces.
 if(s.indexOf(' ')>=0) {
  uint32_t value=0;unsigned count=0;int pos=0;
  while(pos<s.length()) {
   while(pos<s.length() && s[pos]==' ')++pos;
   if(pos>=s.length())break;
   int end=s.indexOf(' ',pos);if(end<0)end=s.length();
   String part=s.substring(pos,end);if(part.length()!=2 || count>=4)return false;
   for(unsigned i=0;i<2;i++)if(!isxdigit((unsigned char)part[i]))return false;
   value |= ((uint32_t)strtoul(part.c_str(),nullptr,16))<<(8*count++);
   pos=end+1;
  }
  if(!count)return false;out=value;return true;
 }
 char *end=nullptr;unsigned long n=strtoul(s.c_str(),&end,0);
 if(*end)return false;out=n;return true;
}
inline bool parse(const String &source,std::vector<Signal>&signals,String &error) {
 if(source.length()>32000){error="file_too_large";return false;}
 Signal current;bool active=false;int pos=0;
 auto flush=[&]()->bool {if(!active)return true;if(current.name.isEmpty()){error="missing_name";return false;}if(current.protocol=="RAW" && (current.timings.size()<2||current.timings.size()%2)){error="invalid_raw_timings";return false;}if(current.protocol!="RAW" && current.protocol!="NEC" && current.protocol!="SAMSUNG32" && current.protocol!="SIRC" && current.protocol!="SIRC15" && current.protocol!="NECEXT" && current.protocol!="KASEIKYO"){error="unsupported_protocol_"+current.protocol;return false;}signals.push_back(current);current=Signal();active=false;return signals.size()<=100;};
 while(pos<source.length()){int end=source.indexOf('\n',pos);if(end<0)end=source.length();String line=source.substring(pos,end);pos=end+1;line.trim();if(!line.length()||line.startsWith("#")||line.startsWith("Filetype:")||line.startsWith("Version:"))continue;
 if(line=="#"){continue;}int colon=line.indexOf(':');if(colon<0)continue;String key=line.substring(0,colon),val=line.substring(colon+1);key.trim();val.trim();
 if(key=="name"){if(!flush())return false;active=true;current.name=val;}
 else if(key=="type"){current.protocol=val.equalsIgnoreCase("raw")?"RAW":"";}
 else if(key=="protocol"){current.protocol=val;current.protocol.toUpperCase();}
 else if(key=="address"){if(!parseNumber(val,current.address)){error="bad_address";return false;}}
 else if(key=="command"){if(!parseNumber(val,current.command)){error="bad_command";return false;}}
 else if(key=="frequency"){if(!parseNumber(val,current.frequency)||current.frequency<20000||current.frequency>60000){error="bad_frequency";return false;}}
 else if(key=="data"){int p=0;while(p<val.length()){while(p<val.length()&&val[p]==' ')p++;if(p>=val.length())break;int q=val.indexOf(' ',p);if(q<0)q=val.length();uint32_t v;if(!parseNumber(val.substring(p,q),v)||v<1||v>65535||current.timings.size()>=512){error="bad_raw_data";return false;}current.timings.push_back((uint16_t)v);p=q+1;}}
 }
 if(!flush())return false;if(signals.empty()){error="no_signals";return false;}return true;
}
inline bool send(const Signal &s,String &error) {
 if(s.protocol=="NEC"){if(s.address>65535||s.command>255){error="nec_values_out_of_range";return false;}IrSender.sendNEC((uint16_t)s.address,(uint8_t)s.command,0);return true;}
 if(s.protocol=="SAMSUNG32"){if(s.address>65535||s.command>255){error="samsung_values_out_of_range";return false;}IrSender.sendSamsung((uint16_t)s.address,(uint8_t)s.command,0);return true;}
 if(s.protocol=="RAW"){if(s.timings.empty()){error="empty_raw";return false;}IrSender.sendRaw(s.timings.data(),s.timings.size(),(uint_fast8_t)(s.frequency/1000));return true;}
 error="unsupported_protocol";return false;
}
}
