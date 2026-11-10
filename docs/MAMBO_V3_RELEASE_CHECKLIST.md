# Mambo OS V3 — engineering and release status

## Implemented in this development branch
- [x] M5StickC Plus2 firmware with web dashboard and USB JSON serial transport (115200 baud).
- [x] Native Android companion source, explicit USB host permission, reconnect polling, USB status, radio status, mode selection, and JSON response view; voice removed.
- [x] GitHub Actions firmware and Android build definitions.
- [x] Shared USB/HTTP radio diagnostics with explicit detection and radio-transmission limitations.
- [x] Persisted single-module preference: none / CC1101 / nRF24.
- [x] Read-only SPI chip-status probe, **disabled in default builds** and gated by verified compile-time GPIO assignments.
- [x] ESP32 pin-role validation for radio SPI outputs, including protection for flash-bus pins 6-11.
- [x] Optional compile-gated CC1101 433.05–434.79 MHz RSSI scan (1–16 bins, uncalibrated until hardware verified).
- [x] Optional compile-gated nRF24 receive-only RPD sampling across 1-16 requested channels (binary threshold, not RSSI).
- [x] USB and HTTP routes and Android/web controls for both measurement paths.
- [x] Web UI offline observation analysis from bounded imported JSON, plus .sub metadata inspection.
- [x] Python source-level regression checks for transport integration, probe gate, Android controls and diagnostics.
- [x] CI JavaScript syntax check for the firmware-embedded dashboard, including Radio Lab.

## Not implemented / not proved
- [ ] Safe M5StickC Plus2 GPIO and power profile for *this exact physical socket* (supplied sketch includes optional transistor, SD hardware, and 5V which are excluded from the final design).
- [ ] Read-only hardware probe enabled in a physically validated build.
- [ ] Confirm CC1101 bounded RSSI sweep and nRF24 RPD actually work on this physical board; optional drivers have not been hardware-tested.
- [ ] Calibrate and validate the bounded CC1101 RSSI sweep on an actual module and antenna; 26 MHz crystal is assumed.
- [ ] Actual hardware module detection on both swappable radios, with physical wiring and voltage validation.
- [ ] Complete USB parity for every legacy web dashboard feature; current USB controls focus on device and radio status/selection.
- [ ] Authentication and access control for management channels.
- [ ] Verified original Mambo artwork in Android and web dashboard.
- [ ] On-device smoke tests after flashing and app installation.
- [ ] Latest CI success confirmation after these new commits.

## Hardware scope
Exactly one M5StickC Plus2, and either CC1101 or nRF24L01+ in the same user-confirmed eight-position socket. No separate SD card, BC547 transistor, external memory or additional microcontroller. The radio must be unplugged/swapped only while **power is off**. It must be supplied with regulated 3.3V within the device's current budget.

## Default probe safety
Default build has NO external radio GPIO probing, because the socket's actual M5StickC Plus2 SCK/MISO/MOSI/CSN/CE-GDO0 wiring and supply have not yet been electrically validated. Both web and USB can request a probe; they receive `hardware_probe_disabled`.

After physical validation, a custom PlatformIO environment can define:
`MAMBO_RADIO_HW_VERIFIED`, `MAMBO_RADIO_PIN_SCK`, `MAMBO_RADIO_PIN_MISO`, `MAMBO_RADIO_PIN_MOSI`, `MAMBO_RADIO_PIN_CSN`, `MAMBO_RADIO_PIN_CONTROL`, optionally `MAMBO_RADIO_PIN_IRQ`. Do **not** copy pin assignments from the illustrative wiring image without checking them against the actual connector.

**The default build does not read the external radios.** An explicitly verified custom build contains CC1101 bounded RSSI sweep and nRF24 RPD sampling code; these drivers are untested on physical hardware. No packet capture, RF transmission, rolling-code credential recovery, or access-control replay is provided. Successful compilation alone is not a hardware acceptance test.
