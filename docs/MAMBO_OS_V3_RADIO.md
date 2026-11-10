# Mambo OS V3 — Dual Radio Integration

Development branch: `feature/mambo-os-dual-radio`

## Target
M5StickC Plus2 firmware + Android companion + browser Radio Lab.
Branding: use the user-supplied MAMBO OS cyan/magenta cat artwork, preserving the original.

## Hardware validation gate
- CC1101 PA/LNA: confirm board variant, operating band, pinout, supply and peak current.
- nRF24L01+ PA/LNA: confirm board pinout, supply and peak current.
- Both radios are 3.3 V logic; use a separately rated 3.3 V supply, common ground and local decoupling.
- SPI SCK/MOSI/MISO may be shared, with separate CS pins; nRF24 also needs CE, and IRQ is optional.
- Verify Plus2 GPIO allocation against display, IR and boot strapping before fixing pin numbers.
- No transmit function should be enabled without confirmed RF hardware and region settings.

## Firmware modules
- SPI bus arbiter and module-presence checks
- CC1101 RSSI sweep and compatible packet reception
- nRF24 channel occupancy and link diagnostics
- LittleFS capture metadata and .sub file import/export
- Authenticated local API; report actual execution results, not only command receipt

## File support
`include/mambo_sub_parser.h` provides a bounded read-only Flipper .sub metadata parser.
It validates Filetype, Frequency and Protocol and recognizes RAW_Data.
This is NOT an RF decoder or transmitter. Parsing a file does not imply that its
modulation or protocol can be transmitted by the attached hardware.

## Android / web
- Spectrum view with measured RSSI (never synthetic values presented as measurements)
- Receiver and saved-signal list
- File picker and metadata preview
- Module status, logs and connectivity
- Show unsupported hardware or functions as unavailable, not as working buttons

## Integration acceptance
1. Firmware and Android build successfully from the same commit.
2. Device boots with neither external module connected.
3. CC1101 and nRF24 are independently detected when wired.
4. RSSI/channel graphs display measured data.
5. Valid .sub metadata imports; malformed and oversized files are rejected.
6. Android/web status matches firmware and reflects errors.
7. Hardware testing confirms power stability and no GPIO conflicts.

Existing `build.yml` only builds PlatformIO firmware. Android workflow location and
build commands must be verified before combining them into one release workflow.
