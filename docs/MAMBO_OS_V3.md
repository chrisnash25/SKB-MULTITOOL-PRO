# Mambo OS V3 — Dual Radio Integration

This branch contains a **read-only hardware-detection foundation**, not a finished firmware, APK or verified hardware release.

## Hardware
- M5StickC Plus2 uses an ESP32 and requires a verified GPIO expansion pin map.
- CC1101 and nRF24L01+ share SCK/MISO/MOSI but require separate CS pins; nRF24 additionally requires CE.
- Both modules need a stable 3.3 V supply. PA/LNA variants may draw more current than the host board can provide.
- Do not connect modules or enable the diagnostic header until wiring and voltage levels have been verified.
- `MamboRadio::Monitor::begin(pins)` performs register/status reads only. It does not transmit.

## Radio Lab roadmap
1. Hardware pin validation and module detection
2. Receive-only CC1101 swept RSSI visualization and nRF24 channel activity
3. Signal library and safe .sub metadata parsing
4. Android/web read-only telemetry and authenticated management
5. Bench-only rolling-code format analysis using synthetic sequences

Rolling-code diagnostics must not extract live access credentials, replay access-control signals or manipulate synchronization counters.

## Integration note
Existing firmware routes, IR features and branding remain unchanged. The monitor header is deliberately not wired into startup until a verified pin map is available.
