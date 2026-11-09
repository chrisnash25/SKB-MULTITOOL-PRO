#pragma once
#include <LittleFS.h>
// IRDB presets sourced from chrisnash25/Flipper-IRDB; retain attribution in upstream files.
inline void installBundledIRProfiles(){
 if(!LittleFS.exists("/ir"))LittleFS.mkdir("/ir");
 if(!LittleFS.exists("/ir/Panasonic_Unknown_Full.ir")){File f=LittleFS.open("/ir/Panasonic_Unknown_Full.ir","w");if(f){f.print(R"SKBIR0(Filetype: IR signals file
Version: 1
# 
name: Power
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: D0 03 00 00
# 
name: 3d
type: parsed
protocol: Kaseikyo
address: 89 02 20 02
command: 70 03 00 00
# 
name: Tv
type: parsed
protocol: Kaseikyo
address: 82 02 20 00
command: 00 03 00 00
# 
name: Av
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 50 00 00 00
# 
name: Menu
type: parsed
protocol: Kaseikyo
address: 80 02 20 01
command: 20 01 00 00
# 
name: Text
type: parsed
protocol: Kaseikyo
address: 81 02 20 00
command: 30 00 00 00
# 
name: Sttl
type: parsed
protocol: Kaseikyo
address: 81 02 20 00
command: 50 00 00 00
# 
name: Aspect
type: parsed
protocol: Kaseikyo
address: 84 02 20 03
command: E0 01 00 00
# 
name: Info
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 90 03 00 00
# 
name: Exit
type: parsed
protocol: Kaseikyo
address: 80 02 20 03
command: 30 01 00 00
# 
name: Apps
type: parsed
protocol: Kaseikyo
address: 89 02 20 02
command: F0 00 00 00
# 
name: Home
type: parsed
protocol: Kaseikyo
address: 89 02 20 02
command: 50 01 00 00
# 
name: Guide
type: parsed
protocol: Kaseikyo
address: 89 02 20 02
command: 70 00 00 00
# 
name: Up
type: parsed
protocol: Kaseikyo
address: 80 02 20 01
command: A0 00 00 00
# 
name: Down
type: parsed
protocol: Kaseikyo
address: 80 02 20 01
command: B0 00 00 00
# 
name: Left
type: parsed
protocol: Kaseikyo
address: 80 02 20 01
command: E0 00 00 00
# 
name: Right
type: parsed
protocol: Kaseikyo
address: 80 02 20 01
command: F0 00 00 00
# 
name: Ok
type: parsed
protocol: Kaseikyo
address: 80 02 20 01
command: 90 00 00 00
# 
name: Option
type: parsed
protocol: Kaseikyo
address: 89 02 20 02
command: 70 02 00 00
# 
name: Back_return
type: parsed
protocol: Kaseikyo
address: 80 02 20 03
command: 40 01 00 00
# 
name: Red
type: parsed
protocol: Kaseikyo
address: 80 02 20 01
command: 00 03 00 00
# 
name: Green
type: parsed
protocol: Kaseikyo
address: 80 02 20 01
command: 10 03 00 00
# 
name: Yellow
type: parsed
protocol: Kaseikyo
address: 80 02 20 01
command: 20 03 00 00
# 
name: Blue
type: parsed
protocol: Kaseikyo
address: 80 02 20 01
command: 30 03 00 00
# 
name: Vol_up
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 00 02 00 00
# 
name: Vol_dn
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 10 02 00 00
# 
name: Mute
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 20 03 00 00
# 
name: Ch_next
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 40 03 00 00
# 
name: Ch_prev
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 50 03 00 00
# 
name: 1
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 00 01 00 00
# 
name: 2
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 10 01 00 00
# 
name: 3
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 20 01 00 00
# 
name: 4
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 30 01 00 00
# 
name: 5
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 40 01 00 00
# 
name: 6
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 50 01 00 00
# 
name: 7
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 60 01 00 00
# 
name: 8
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 70 01 00 00
# 
name: 9
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 80 01 00 00
# 
name: Help
type: parsed
protocol: Kaseikyo
address: 80 02 20 02
command: C0 02 00 00
# 
name: 0
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 90 01 00 00
# 
name: Lastview
type: parsed
protocol: Kaseikyo
address: 80 02 20 00
command: 70 03 00 00
# 
name: Back
type: parsed
protocol: Kaseikyo
address: 89 02 20 03
command: 40 00 00 00
# 
name: Play
type: parsed
protocol: Kaseikyo
address: 89 02 20 03
command: 00 00 00 00
# 
name: Forward
type: parsed
protocol: Kaseikyo
address: 89 02 20 03
command: 30 00 00 00
# 
name: Rewind
type: parsed
protocol: Kaseikyo
address: 89 02 20 03
command: C0 01 00 00
# 
name: Pause
type: parsed
protocol: Kaseikyo
address: 89 02 20 03
command: 10 00 00 00
# 
name: Ffwd
type: parsed
protocol: Kaseikyo
address: 89 02 20 03
command: D0 01 00 00
# 
name: Stop
type: parsed
protocol: Kaseikyo
address: 89 02 20 03
command: 20 00 00 00
# 
name: Record
type: parsed
protocol: Kaseikyo
address: 89 02 20 03
command: 50 00 00 00
)SKBIR0");f.close();}}
 if(!LittleFS.exists("/ir/Toshiba_SE_R0305_32CV100U.ir")){File f=LittleFS.open("/ir/Toshiba_SE_R0305_32CV100U.ir","w");if(f){f.print(R"SKBIR1(Filetype: IR signals file
Version: 1
# 
# Remote SE R0305 for LCD HDTV model 32CV100U
#
# Captured by: emptythevoid
#
name: Power
type: parsed
protocol: NEC
address: 40 00 00 00
command: 12 00 00 00
# 
name: Sleep
type: parsed
protocol: NEC
address: 40 00 00 00
command: 15 00 00 00
# 
name: Tv_dvd
type: parsed
protocol: NECext
address: 45 BC 00 00
command: B3 4C 00 00
# 
name: Eject
type: parsed
protocol: NEC
address: 45 00 00 00
command: F5 00 00 00
# 
name: 1
type: parsed
protocol: NEC
address: 40 00 00 00
command: 01 00 00 00
# 
name: 2
type: parsed
protocol: NEC
address: 40 00 00 00
command: 02 00 00 00
# 
name: 3
type: parsed
protocol: NEC
address: 40 00 00 00
command: 03 00 00 00
# 
name: 4
type: parsed
protocol: NEC
address: 40 00 00 00
command: 04 00 00 00
# 
name: 5
type: parsed
protocol: NEC
address: 40 00 00 00
command: 05 00 00 00
# 
name: 6
type: parsed
protocol: NEC
address: 40 00 00 00
command: 06 00 00 00
# 
name: 7
type: parsed
protocol: NEC
address: 40 00 00 00
command: 07 00 00 00
# 
name: 8
type: parsed
protocol: NEC
address: 40 00 00 00
command: 08 00 00 00
# 
name: 9
type: parsed
protocol: NEC
address: 40 00 00 00
command: 09 00 00 00
# 
name: 0
type: parsed
protocol: NEC
address: 40 00 00 00
command: 00 00 00 00
# 
name: Display
type: parsed
protocol: NEC
address: 40 00 00 00
command: 1C 00 00 00
# 
name: Jump
type: parsed
protocol: NEC
address: 44 00 00 00
command: 9B 00 00 00
# 
name: Subtitle
type: parsed
protocol: NEC
address: 44 00 00 00
command: 87 00 00 00
# 
name: Input_zoom
type: parsed
protocol: NEC
address: 40 00 00 00
command: 0F 00 00 00
# 
name: Audio_select
type: parsed
protocol: NEC
address: 44 00 00 00
command: 53 00 00 00
# 
name: Mute
type: parsed
protocol: NEC
address: 40 00 00 00
command: 10 00 00 00
# 
name: Ch_next
type: parsed
protocol: NEC
address: 40 00 00 00
command: 1B 00 00 00
# 
name: Ch_prev
type: parsed
protocol: NEC
address: 40 00 00 00
command: 1F 00 00 00
# 
name: Vol_up
type: parsed
protocol: NEC
address: 40 00 00 00
command: 1A 00 00 00
# 
name: Vol_dn
type: parsed
protocol: NEC
address: 40 00 00 00
command: 1E 00 00 00
# 
name: Play_mode
type: parsed
protocol: NEC
address: 40 00 00 00
command: 1D 00 00 00
# 
name: Dvd_menu
type: parsed
protocol: NEC
address: 40 00 00 00
command: 19 00 00 00
# 
name: Up
type: parsed
protocol: NEC
address: 44 00 00 00
command: 80 00 00 00
# 
name: Left
type: parsed
protocol: NEC
address: 44 00 00 00
command: 51 00 00 00
# 
name: Right
type: parsed
protocol: NEC
address: 44 00 00 00
command: 4D 00 00 00
# 
name: Down
type: parsed
protocol: NEC
address: 44 00 00 00
command: 81 00 00 00
# 
name: Enter
type: parsed
protocol: NEC
address: 44 00 00 00
command: 4E 00 00 00
# 
name: Menu_setup
type: parsed
protocol: NEC
address: 40 00 00 00
command: 0E 00 00 00
# 
name: Exit_cancel
type: parsed
protocol: NEC
address: 44 00 00 00
command: EF 00 00 00
# 
name: Pause
type: parsed
protocol: NEC
address: 44 00 00 00
command: 10 00 00 00
# 
name: Play
type: parsed
protocol: NEC
address: 44 00 00 00
command: 15 00 00 00
# 
name: Stop
type: parsed
protocol: NEC
address: 44 00 00 00
command: 14 00 00 00
# 
name: Skip_bk
type: parsed
protocol: NEC
address: 40 00 00 00
command: 17 00 00 00
# 
name: Skip_fwd
type: parsed
protocol: NEC
address: 40 00 00 00
command: 57 00 00 00
# 
name: Rev
type: parsed
protocol: NEC
address: 44 00 00 00
command: 19 00 00 00
# 
name: Ff
type: parsed
protocol: NEC
address: 44 00 00 00
command: 13 00 00 00
# 
name: Slow_bk
type: parsed
protocol: NEC
address: 45 00 00 00
command: 0E 00 00 00
# 
name: Slow_fwd
type: parsed
protocol: NEC
address: 44 00 00 00
command: 0D 00 00 00
# 
name: Angle
type: parsed
protocol: NEC
address: 44 00 00 00
command: 96 00 00 00
# 
name: Marker
type: parsed
protocol: NEC
address: 45 00 00 00
command: EC 00 00 00
# 
name: Top_menu
type: parsed
protocol: NEC
address: 44 00 00 00
command: DF 00 00 00
# 
name: Return
type: parsed
protocol: NEC
address: 44 00 00 00
command: 5D 00 00 00
# 
name: Repeat_a_b
type: parsed
protocol: NEC
address: 44 00 00 00
command: 5C 00 00 00
# 
name: Game
type: parsed
protocol: NEC
address: 40 00 00 00
command: 28 00 00 00
# 
name: Pic_size
type: parsed
protocol: NEC
address: 40 00 00 00
command: 59 00 00 00
)SKBIR1");f.close();}}
 if(!LittleFS.exists("/ir/SonyTV.ir")){File f=LittleFS.open("/ir/SonyTV.ir","w");if(f){f.print(R"SKBIR2(Filetype: IR signals file
Version: 1
# 
name: On_off
type: parsed
protocol: SIRC
address: 01 00 00 00
command: 15 00 00 00
# 
name: Netflix
type: parsed
protocol: SIRC15
address: 1A 00 00 00
command: 7C 00 00 00
# 
name: Up
type: parsed
protocol: SIRC
address: 01 00 00 00
command: 74 00 00 00
# 
name: Down
type: parsed
protocol: SIRC
address: 01 00 00 00
command: 75 00 00 00
# 
name: Left
type: parsed
protocol: SIRC
address: 01 00 00 00
command: 34 00 00 00
# 
name: Right
type: parsed
protocol: SIRC
address: 01 00 00 00
command: 33 00 00 00
# 
name: Ok
type: parsed
protocol: SIRC
address: 01 00 00 00
command: 65 00 00 00
# 
name: Back
type: parsed
protocol: SIRC15
address: 97 00 00 00
command: 23 00 00 00
# 
name: Vol_up
type: parsed
protocol: SIRC
address: 01 00 00 00
command: 12 00 00 00
# 
name: Vol_dn
type: parsed
protocol: SIRC
address: 01 00 00 00
command: 13 00 00 00
# 
name: Mute
type: parsed
protocol: SIRC
address: 01 00 00 00
command: 14 00 00 00
# 
name: Home
type: parsed
protocol: SIRC
address: 01 00 00 00
command: 60 00 00 00
# 
name: Hdmi
type: parsed
protocol: SIRC
address: 01 00 00 00
command: 25 00 00 00
)SKBIR2");f.close();}}
 if(!LittleFS.exists("/ir/Sharp_Roku_TV.ir")){File f=LittleFS.open("/ir/Sharp_Roku_TV.ir","w");if(f){f.print(R"SKBIR3(Filetype: IR signals file
Version: 1
# 
name: Power
type: parsed
protocol: NEC
address: 04 00 00 00
command: 08 00 00 00
# 
name: Back
type: parsed
protocol: NEC
address: 04 00 00 00
command: 04 00 00 00
# 
name: Home
type: parsed
protocol: NEC
address: 04 00 00 00
command: 43 00 00 00
# 
name: Vol_up
type: parsed
protocol: NEC
address: 04 00 00 00
command: 02 00 00 00
# 
name: Vol_dn
type: parsed
protocol: NEC
address: 04 00 00 00
command: 03 00 00 00
# 
name: Mute
type: parsed
protocol: NEC
address: 04 00 00 00
command: 09 00 00 00
# 
name: Up
type: parsed
protocol: NEC
address: 04 00 00 00
command: 56 00 00 00
# 
name: Down
type: parsed
protocol: NEC
address: 04 00 00 00
command: 57 00 00 00
# 
name: Left
type: parsed
protocol: NEC
address: 04 00 00 00
command: 58 00 00 00
# 
name: Right
type: parsed
protocol: NEC
address: 04 00 00 00
command: 59 00 00 00
# 
name: Ok
type: parsed
protocol: NEC
address: 04 00 00 00
command: 5A 00 00 00
# 
name: Again
type: parsed
protocol: NEC
address: 04 00 00 00
command: 41 00 00 00
# 
name: Sleep
type: parsed
protocol: NEC
address: 04 00 00 00
command: 1A 00 00 00
# 
name: Star
type: parsed
protocol: NEC
address: 04 00 00 00
command: 0C 00 00 00
# 
name: Rewind
type: parsed
protocol: NEC
address: 04 00 00 00
command: 4F 00 00 00
# 
name: Play_pause
type: parsed
protocol: NEC
address: 04 00 00 00
command: 42 00 00 00
# 
name: Fast_forward
type: parsed
protocol: NEC
address: 04 00 00 00
command: 5B 00 00 00
# 
name: Netflix
type: parsed
protocol: NEC
address: 04 00 00 00
command: 4C 00 00 00
# 
name: Hulu
type: parsed
protocol: NEC
address: 04 00 00 00
command: 5C 00 00 00
# 
name: Sling
type: parsed
protocol: NEC
address: 04 00 00 00
command: 45 00 00 00
# 
name: Now
type: parsed
protocol: NEC
address: 04 00 00 00
command: 67 00 00 00
)SKBIR3");f.close();}}
 if(!LittleFS.exists("/ir/LG_Full.ir")){File f=LittleFS.open("/ir/LG_Full.ir","w");if(f){f.print(R"SKBIR4(Filetype: IR signals file
Version: 1
#
# For LG TV Unknown Model
# 
name: Power
type: parsed
protocol: NEC
address: 04 00 00 00
command: 08 00 00 00
# 
name: Mute
type: parsed
protocol: NEC
address: 04 00 00 00
command: 09 00 00 00
# 
name: Vol_up
type: parsed
protocol: NEC
address: 04 00 00 00
command: 02 00 00 00
# 
name: Ch_next
type: parsed
protocol: NEC
address: 04 00 00 00
command: 00 00 00 00
# 
name: Vol_dn
type: parsed
protocol: NEC
address: 04 00 00 00
command: 03 00 00 00
# 
name: Ch_prev
type: parsed
protocol: NEC
address: 04 00 00 00
command: 01 00 00 00
)SKBIR4");f.close();}}
 if(!LittleFS.exists("/ir/Bush_VL32HDLED.ir")){File f=LittleFS.open("/ir/Bush_VL32HDLED.ir","w");if(f){f.print(R"SKBIR5(Filetype: IR signals file
Version: 1
# 
# Bush TV VL32HDLED
#
name: Power
type: parsed
protocol: NEC
address: 08 00 00 00
command: D7 00 00 00
# 
name: Vol_up
type: parsed
protocol: NEC
address: 08 00 00 00
command: 80 00 00 00
# 
name: Vol_dn
type: parsed
protocol: NEC
address: 08 00 00 00
command: 8E 00 00 00
# 
name: Ch_next
type: parsed
protocol: NEC
address: 08 00 00 00
command: 83 00 00 00
# 
name: Ch_prev
type: parsed
protocol: NEC
address: 08 00 00 00
command: 86 00 00 00
#
name: Prev_ch
type: parsed
protocol: NEC
address: 08 00 00 00
command: DD 00 00 00
# 
name: Source
type: parsed
protocol: NEC
address: 08 00 00 00
command: B1 00 00 00
# 
name: Up
type: parsed
protocol: NEC
address: 08 00 00 00
command: 92 00 00 00
# 
name: Down
type: parsed
protocol: NEC
address: 08 00 00 00
command: D8 00 00 00
# 
name: Left
type: parsed
protocol: NEC
address: 08 00 00 00
command: 97 00 00 00
# 
name: Right
type: parsed
protocol: NEC
address: 08 00 00 00
command: 9F 00 00 00
# 
name: Ok
type: parsed
protocol: NEC
address: 08 00 00 00
command: 9B 00 00 00
# 
name: Menu
type: parsed
protocol: NEC
address: 08 00 00 00
command: D4 00 00 00
# 
name: Exit
type: parsed
protocol: NEC
address: 08 00 00 00
command: DC 00 00 00
# 
name: Mute
type: parsed
protocol: NEC
address: 08 00 00 00
command: DF 00 00 00
# 
name: Sleep
type: parsed
protocol: NEC
address: 08 00 00 00
command: D5 00 00 00
# 
name: Subtitle
type: parsed
protocol: NEC
address: 08 00 00 00
command: D1 00 00 00
# 
name: 1
type: parsed
protocol: NEC
address: 08 00 00 00
command: 95 00 00 00
# 
name: 2
type: parsed
protocol: NEC
address: 08 00 00 00
command: 99 00 00 00
# 
name: 3
type: parsed
protocol: NEC
address: 08 00 00 00
command: 9D 00 00 00
# 
name: 4
type: parsed
protocol: NEC
address: 08 00 00 00
command: D6 00 00 00
# 
name: 5
type: parsed
protocol: NEC
address: 08 00 00 00
command: DA 00 00 00
# 
name: 6
type: parsed
protocol: NEC
address: 08 00 00 00
command: DE 00 00 00
# 
name: 7
type: parsed
protocol: NEC
address: 08 00 00 00
command: 96 00 00 00
# 
name: 8
type: parsed
protocol: NEC
address: 08 00 00 00
command: 9A 00 00 00
# 
name: 9
type: parsed
protocol: NEC
address: 08 00 00 00
command: 9E 00 00 00
# 
name: 0
type: parsed
protocol: NEC
address: 08 00 00 00
command: D9 00 00 00
# 
name: Usb
type: parsed
protocol: NEC
address: 08 00 00 00
command: 8C 00 00 00
# 
name: Pmode
type: parsed
protocol: NEC
address: 08 00 00 00
command: C2 00 00 00
# 
name: Smode
type: parsed
protocol: NEC
address: 08 00 00 00
command: C1 00 00 00
# 
name: Audio
type: parsed
protocol: NEC
address: 08 00 00 00
command: C0 00 00 00
# 
name: Tv
type: parsed
protocol: NEC
address: 08 00 00 00
command: C3 00 00 00
# 
name: Info
type: parsed
protocol: NEC
address: 08 00 00 00
command: 8A 00 00 00
# 
name: Ch-list
type: parsed
protocol: NEC
address: 08 00 00 00
command: A7 00 00 00
# 
name: Freeze
type: parsed
protocol: NEC
address: 08 00 00 00
command: DB 00 00 00
# 
name: Aspect
type: parsed
protocol: NEC
address: 08 00 00 00
command: 82 00 00 00
# 
name: Fav
type: parsed
protocol: NEC
address: 08 00 00 00
command: 87 00 00 00
# 
name: Fav+
type: parsed
protocol: NEC
address: 08 00 00 00
command: 8B 00 00 00
# 
name: Fav-
type: parsed
protocol: NEC
address: 08 00 00 00
command: 8F 00 00 00
# 
name: Index
type: parsed
protocol: NEC
address: 08 00 00 00
command: C7 00 00 00
# 
name: Text
type: parsed
protocol: NEC
address: 08 00 00 00
command: 93 00 00 00
# 
name: Play/pause
type: parsed
protocol: NEC
address: 08 00 00 00
command: C8 00 00 00
# 
name: Stop
type: parsed
protocol: NEC
address: 08 00 00 00
command: 81 00 00 00
# 
name: Record
type: parsed
protocol: NEC
address: 08 00 00 00
command: CC 00 00 00
# 
name: Subpage
type: parsed
protocol: NEC
address: 08 00 00 00
command: D2 00 00 00
# 
name: Red
type: parsed
protocol: NEC
address: 08 00 00 00
command: 84 00 00 00
# 
name: Green
type: parsed
protocol: NEC
address: 08 00 00 00
command: 88 00 00 00
# 
name: Yellow
type: parsed
protocol: NEC
address: 08 00 00 00
command: 94 00 00 00
# 
name: Blue
type: parsed
protocol: NEC
address: 08 00 00 00
command: 90 00 00 00
)SKBIR5");f.close();}}
}
