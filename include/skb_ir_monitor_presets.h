#pragma once
#include <LittleFS.h>
// Power-only controls obtained from chrisnash25/Flipper-IRDB/Monitors, 2026-11-09.
// These records are installed on first boot without replacing user-imported profiles.
inline void installMonitorPowerPresets(){
 if(!LittleFS.exists("/ir")) LittleFS.mkdir("/ir");
 if(!LittleFS.exists("/ir/Monitor_Aorus_FV43U.ir")){File f=LittleFS.open("/ir/Monitor_Aorus_FV43U.ir","w");if(f){f.print(R"M0(Filetype: IR signals file
Version: 1
# Original source: Monitors/Aorus/Aorus_FV43U.ir
name: Power
type: parsed
protocol: NEC
address: 00 00 00 00
command: 1A 00 00 00
)M0");f.close();}}
 if(!LittleFS.exists("/ir/Monitor_BenQ_EX2780Q.ir")){File f=LittleFS.open("/ir/Monitor_BenQ_EX2780Q.ir","w");if(f){f.print(R"M1(Filetype: IR signals file
Version: 1
# Original source: Monitors/BenQ/BenQ_EX2780Q.ir
name: Power
type: parsed
protocol: NECext
address: 60 30 00 00
command: 02 FD 00 00
)M1");f.close();}}
 if(!LittleFS.exists("/ir/Monitor_Clinton_Electronics_M10A.ir")){File f=LittleFS.open("/ir/Monitor_Clinton_Electronics_M10A.ir","w");if(f){f.print(R"M2(Filetype: IR signals file
Version: 1
# Original source: Monitors/Clinton_Electronics/Clinton_Electronics_M-10A.ir
name: Power
type: parsed
protocol: NECext
address: 42 BF 00 00
command: 12 ED 00 00
)M2");f.close();}}
 if(!LittleFS.exists("/ir/Monitor_LG_43UD79-B.ir")){File f=LittleFS.open("/ir/Monitor_LG_43UD79-B.ir","w");if(f){f.print(R"M3(Filetype: IR signals file
Version: 1
# Original source: Monitors/LG/LG_43UD79-B.ir
name: Power
type: parsed
protocol: NEC
address: 04 00 00 00
command: 08 00 00 00
)M3");f.close();}}
 if(!LittleFS.exists("/ir/Monitor_LG_UltraGear_OLED.ir")){File f=LittleFS.open("/ir/Monitor_LG_UltraGear_OLED.ir","w");if(f){f.print(R"M4(Filetype: IR signals file
Version: 1
# Original source: Monitors/LG/LG_UltraGear_OLED_Monitor.ir
name: Power
type: parsed
protocol: NECext
address: 04 F4 00 00
command: 08 F7 00 00
)M4");f.close();}}
 if(!LittleFS.exists("/ir/Monitor_Loncevon_12inch.ir")){File f=LittleFS.open("/ir/Monitor_Loncevon_12inch.ir","w");if(f){f.print(R"M5(Filetype: IR signals file
Version: 1
# Original source: Monitors/Loncevon/LONCEVON-12_inch_Small_Monitor.ir
name: Power
type: parsed
protocol: NEC
address: 00 00 00 00
command: 00 00 00 00
)M5");f.close();}}
 if(!LittleFS.exists("/ir/Monitor_Monoprice_43305.ir")){File f=LittleFS.open("/ir/Monitor_Monoprice_43305.ir","w");if(f){f.print(R"M6(Filetype: IR signals file
Version: 1
# Original source: Monitors/Monoprice/Monoprice_43305.ir
name: Power
type: parsed
protocol: NEC
address: 20 00 00 00
command: 41 00 00 00
)M6");f.close();}}
 if(!LittleFS.exists("/ir/Monitor_Philips_Evnia_49M2C8900L.ir")){File f=LittleFS.open("/ir/Monitor_Philips_Evnia_49M2C8900L.ir","w");if(f){f.print(R"M7(Filetype: IR signals file
Version: 1
# Original source: Monitors/Philips/Philips_Evnia_49M2C8900L.ir
name: Power
type: parsed
protocol: RC6
address: 00 00 00 00
command: 0C 00 00 00
)M7");f.close();}}
}
