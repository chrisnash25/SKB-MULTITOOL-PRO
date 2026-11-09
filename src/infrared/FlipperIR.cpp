#include "FlipperIR.h"
#include <stdlib.h>
static uint64_t flipperHex(const String& value, bool& ok) {
  String digits;
  for (unsigned i=0; i<value.length(); ++i) {
    char c=value[i];
    if (c==' ' || c=='\t') continue;
    if (!isxdigit(static_cast<unsigned char>(c)) || digits.length()>=16) { ok=false; return 0; }
    digits += c;
  }
  if (digits.isEmpty() || (digits.length()%2)!=0) {ok=false;return 0;}
  ok=true;
  return strtoull(digits.c_str(),nullptr,16);
}
static String valueOf(const String& s) { int p=s.indexOf(':'); if(p<0)return ""; String v=s.substring(p+1);v.trim();return v; }
bool parseFlipperIR(fs::FS& fs,const String& path,std::vector<IRCommand>& out,String& error){
 out.clear();
 if(!path.startsWith("/ir/") || !path.endsWith(".ir") || path.indexOf("..")>=0){error="invalid_path";return false;}
 File f=fs.open(path,"r"); if(!f){error="file_not_found";return false;}
 if(f.size()>65536){f.close();error="file_too_large";return false;}
 IRCommand cur; bool active=false, hasType=false;
 auto commit=[&]()->bool {
  if(!active)return true;
  if(cur.name.isEmpty()||!hasType){error="missing_fields";return false;}
  if(cur.raw && (cur.timings.empty() || cur.timings.size()>2048)){error="invalid_raw";return false;}
  if(out.size()>=128){error="too_many_commands";return false;}
  out.push_back(cur);return true;
 };
 while(f.available()){
  String line=f.readStringUntil('\n');line.trim();
  if(line.isEmpty()||line.startsWith("#")||line.startsWith("Filetype:")||line.startsWith("Version:"))continue;
  int colon=line.indexOf(':');if(colon<0){error="malformed_line";f.close();return false;}
  String key=line.substring(0,colon),v=valueOf(line);key.trim();
  if(key=="name"){
   if(!commit()){f.close();return false;}
   cur=IRCommand();cur.name=v;active=true;hasType=false;
  } else if(!active)continue;
  else if(key=="type"){cur.raw=(v=="raw");hasType=(v=="raw"||v=="parsed");if(!hasType){error="unsupported_type";f.close();return false;}}
  else if(key=="protocol")cur.protocol=v;
  else if(key=="address"){bool ok=false;cur.address=flipperHex(v,ok);if(!ok){error="invalid_address";f.close();return false;}}
  else if(key=="command"){bool ok=false;cur.command=flipperHex(v,ok);if(!ok){error="invalid_command";f.close();return false;}}
  else if(key=="frequency"){long hz=v.toInt();if(hz<20000||hz>60000){error="invalid_frequency";f.close();return false;}cur.frequency=hz;}
  else if(key=="data"){
   char* end=nullptr;const char* p=v.c_str();
   while(*p){while(*p==' ')p++;if(!*p)break;unsigned long n=strtoul(p,&end,10);
    if(end==p||n==0||n>65535||cur.timings.size()>=2048){error="invalid_timings";f.close();return false;}
    cur.timings.push_back((uint16_t)n);p=end;
   }
  }
 }
 f.close();return commit();
}
