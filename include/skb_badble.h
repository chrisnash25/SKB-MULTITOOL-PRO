#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <BleKeyboard.h>
// Keyboard HID is used only after the host's standard Bluetooth pairing approval.
// Automatic BLE advertising/reconnection; NO automatic keystroke transmission.
static BleKeyboard skbKeyboard("SKB BLE Keyboard","SKB",100);
static bool skbKeyboardStarted=false;
static bool skbMacroRunning=false;
static bool skbMacroStop=false;
static String skbMacroCurrent="";
static size_t skbMacroPosition=0;
static unsigned long skbMacroNextMs=0;
static Preferences skbMacroPrefs;

inline void skbBadBleInit(){
 skbMacroPrefs.begin("skb-hid",false);
 skbKeyboard.begin();skbKeyboardStarted=true;
}
inline bool skbMacroValidName(const String& name){
 if(!name.length()||name.length()>20)return false;
 for(unsigned i=0;i<name.length();i++) if(!isalnum((unsigned char)name[i])&&name[i]!='_'&&name[i]!='-')return false;
 return true;
}
inline bool skbMacroValidText(const String& value){
 if(!value.length()||value.length()>160)return false;
 for(unsigned i=0;i<value.length();i++){uint8_t c=(uint8_t)value[i];if(c<32||c>126)return false;}
 return true;
}
inline String skbMacroIndex(){
 return skbMacroPrefs.getString("names","");
}
inline bool skbMacroExists(const String& name){
 String names=","+skbMacroIndex()+",";
 return names.indexOf(","+name+",")>=0;
}
inline void skbBadBleStatus(JsonDocument& doc){
 doc["ok"]=true;doc["mode"]="BLE HID keyboard";doc["device"]="SKB BLE Keyboard";
 doc["started"]=skbKeyboardStarted;doc["connected"]=skbKeyboard.isConnected();
 doc["running"]=skbMacroRunning;doc["selected"]=skbMacroCurrent;
 doc["position"]=skbMacroPosition;doc["manual_execution_only"]=true;
 doc["note"]="Pair once on the host; keyboard reconnects when the host allows it. No macros run on pairing.";
}
inline void skbBadBleList(JsonDocument& doc){
 doc["ok"]=true;
 JsonArray items=doc["macros"].to<JsonArray>();
 String names=skbMacroIndex();int start=0;
 while(start<names.length()){
  int end=names.indexOf(',',start);if(end<0)end=names.length();
  String name=names.substring(start,end);if(skbMacroValidName(name)){
   JsonObject m=items.add<JsonObject>();m["name"]=name;
   m["text"]=skbMacroPrefs.getString(("m_"+name).c_str(),"");
  }
  start=end+1;
 }
}
inline void skbBadBleCommand(JsonDocument& req,JsonDocument& res){
 String action=req["action"] | "status";
 if(action=="status")skbBadBleStatus(res);
 else if(action=="list")skbBadBleList(res);
 else if(action=="save"){
  String name=req["name"] | "",value=req["text"] | "";
  if(!skbMacroValidName(name)||!skbMacroValidText(value)){res["ok"]=false;res["error"]="name_1_to_20_ascii_or_text_1_to_160_printable_ascii";return;}
  String names=skbMacroIndex();
  if(!skbMacroExists(name)){
   unsigned count=0;for(unsigned i=0;i<names.length();i++)if(names[i]==',')count++;
   if(names.length())count++;
   if(count>=8){res["ok"]=false;res["error"]="macro_limit_8";return;}
   names+=names.length()?",":"";names+=name;
   skbMacroPrefs.putString("names",names);
  }
  skbMacroPrefs.putString(("m_"+name).c_str(),value);res["ok"]=true;res["name"]=name;
 }else if(action=="delete"){
  String name=req["name"] | "";
  if(skbMacroRunning){res["ok"]=false;res["error"]="stop_macro_first";return;}
  if(!skbMacroExists(name)){res["ok"]=false;res["error"]="macro_not_found";return;}
  String names=skbMacroIndex(),next="";int st=0;
  while(st<names.length()){
   int en=names.indexOf(',',st);if(en<0)en=names.length();
   String part=names.substring(st,en);
   if(part!=name){if(next.length())next+=",";next+=part;}
   st=en+1;
  }
  skbMacroPrefs.putString("names",next);
  skbMacroPrefs.remove(("m_"+name).c_str());
  res["ok"]=true;
 }else if(action=="run"){
  String name=req["name"] | "";
  if(skbMacroRunning){res["ok"]=false;res["error"]="macro_already_running";return;}
  if(!skbKeyboard.isConnected()){res["ok"]=false;res["error"]="keyboard_not_paired_or_connected";return;}
  if(!skbMacroExists(name)){res["ok"]=false;res["error"]="macro_not_found";return;}
  skbMacroCurrent=name;skbMacroPosition=0;skbMacroStop=false;
  skbMacroNextMs=millis();skbMacroRunning=true;res["ok"]=true;res["running"]=true;
 }else if(action=="stop"){
  skbMacroStop=true;skbMacroRunning=false;
  skbKeyboard.releaseAll();res["ok"]=true;res["running"]=false;
 }else{res["ok"]=false;res["error"]="unknown_action";}
}
inline void skbBadBleTick(){
 if(!skbMacroRunning)return;
 if(skbMacroStop||!skbKeyboard.isConnected()){skbMacroRunning=false;skbKeyboard.releaseAll();return;}
 if((int32_t)(millis()-skbMacroNextMs)<0)return;
 String text=skbMacroPrefs.getString(("m_"+skbMacroCurrent).c_str(),"");
 if(skbMacroPosition>=text.length()){skbMacroRunning=false;skbKeyboard.releaseAll();return;}
 skbKeyboard.write((uint8_t)text[skbMacroPosition++]);
 skbMacroNextMs=millis()+85; // deliberate rate, nonblocking; STOP remains responsive
}
