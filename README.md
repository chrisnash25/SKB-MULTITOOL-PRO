# SKB MULTITOOL PRO

M5StickC Plus2 firmware project for a phone-first management interface.

## Status
Early development scaffold. Not yet compiled or hardware validated.

## Build
GitHub Actions: Actions > Build firmware > Run workflow.
Or locally: `pio run -e m5stick-cplus2`.

## Hardware
Original ESP32; 135x240 TFT; onboard IR TX; USB-UART serial.
No assumption of PSRAM, SD, IR receiver, or native USB HID.

## Security
Local management AP only. No credential collection, deauthentication transmission, jamming, or unauthorised impersonation.

## Roadmap
Display navigation, LittleFS IR library, BLE scanner/GATT, OTA, and advanced diagnostics require further implementation.
