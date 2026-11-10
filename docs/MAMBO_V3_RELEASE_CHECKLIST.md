# Mambo OS V3 release checklist

## Integrated
- [x] Firmware dashboard Radio Lab tab and module selector.
- [x] Radio selection/status JSON API.
- [x] Read-only .sub metadata inspection API.
- [x] Single interchangeable radio abstraction (CC1101 or nRF24).
- [x] Branch-triggered firmware build workflow.
- [x] Source-level integration checks.

## Not yet validated / not implemented
- [ ] Confirm pin mapping against the actual M5StickC Plus2 connector and both module silkscreens.
- [ ] Verify PA/LNA module 3.3 V supply current under load.
- [ ] Wire the read-only SPI probe into runtime only after pin validation.
- [ ] Test detection and read-only measurements on physical modules.
- [ ] Validate latest firmware binary via GitHub Actions and flash test.
- [ ] Create and build a native Android companion APK.
- [ ] Add offline protocol-analysis fixtures and regression tests.
- [ ] Validate existing Mambo OS branding across app and dashboard.

Radio mode selection is a software preference, **not** proof that the selected module is present. No module should be hot-swapped. No RF transmission or rolling-code replay is implemented by the Radio Lab.
