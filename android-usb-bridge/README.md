# SKB USB Bridge (Android)
Phone-only USB OTG control companion for SKB MULTITOOL PRO on M5StickC Plus2.
The board uses a CH9102 USB-to-serial chip rather than USB networking.
Connect the Samsung S21 USB-C OTG cable to the Stick, open this app, tap CONNECT and approve Android USB access.
Uses 115200 8N1 and newline-delimited USB JSON RPC; keeps the phone's Wi-Fi and mobile-data connection untouched.
The firmware remains accessible over its legacy Wi-Fi AP at http://192.168.4.1 as a fallback.
Build the Android debug APK using GitHub Actions "Build USB Android bridge"; flash a firmware build containing include/skb_usb_protocol.h and its main.cpp integration.
Features: status, TV/monitor remote key selection and one-tap send, Wi-Fi scanning, BLE scans/beacon, PathShield, and USB text IR import.
This is USB serial, not USB tethering or an RNDIS/ECM network interface. Hardware pairing and IR transmission still require on-device verification.
