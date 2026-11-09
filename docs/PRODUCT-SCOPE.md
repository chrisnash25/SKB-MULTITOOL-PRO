# SKB MULTITOOL PRO — Product Scope

## Primary applications
1. Infrared remote hub: Flipper-IRDB catalogue, TV and monitor power controls, favourites, raw and parsed IR support.
2. Wi-Fi toolkit: nearby AP discovery, RSSI, channel analysis, connection diagnostics.
3. Bluetooth toolkit: BLE discovery, advertisement analysis, permitted GATT inspection, controlled peripheral testing.
4. Samsung Galaxy S21 dashboard: responsive authenticated local web UI.
5. Device settings: battery, logs, serial control, firmware updates and storage maintenance.

## Not in scope
Bruce-style offensive tooling, credential capture, deauthentication transmitters, jamming, unrelated hacking applications.

## Universal IR library
Research and import TV and monitor power codes with provenance, supported-protocol checks and deduplication. The catalogue must distinguish Power (toggle), Power_off and Power_on. Avoid claiming a given code works on a model without testing. One-button batch control should use a defined home test set rather than indiscriminately broadcasting power commands to arbitrary equipment.

## Current verified implementation state
- Wi-Fi scan HTTP endpoints committed; not compiled or hardware tested.
- IR parser groundwork committed; Flipper spaced-byte hexadecimal parsing needs correction.
- PowerToggle adapter committed; not connected to the UI or command routing.
- BLE tools and full branded dashboard not implemented.
- No validated release firmware available yet.

## Next development milestones
1. Verify PlatformIO compile in GitHub Actions and correct hardware configuration.
2. Add LittleFS persistence, import validation and IR transmission integration.
3. Add BLE scanner and live dashboard.
4. Add authenticated administrative controls and safe OTA updates.
5. Conduct hardware validation and publish tagged binaries.
