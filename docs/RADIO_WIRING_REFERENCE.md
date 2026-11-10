# Mambo OS V3 — two-module swap-only wiring reference

## Final hardware scope (confirmed by owner)
- One **M5StickC Plus2**.
- One **CC1101** module OR one **nRF24L01+** module installed at a time.
- Power off and disconnect the USB cable before swapping modules.
- **No SD/microSD, BC547 transistor, 5 kΩ resistor, additional MCU, external memory module, or second radio connected simultaneously.** These were drawn in the previous third-party schematic but are **not** part of this build.

## Reference supplied
The supplied illustration depicts a **CC1101 with shared SPI** and other optional parts. It is useful as a reference for that CC1101 configuration; it is **not** proof that the pictured breakout pins can be connected one-for-one to an nRF24 module.

### CC1101 eight header signals
| Header position in supplied diagram | Function |
|---|---|
| 1 | GND |
| 2 | VCC 3.3 V |
| 3 | GDO0 |
| 4 | CSN |
| 5 | SCK |
| 6 | MOSI |
| 7 | MISO |
| 8 | GDO2 |

### Standard nRF24L01+ logical signals
GND, VCC 3.3 V, CE, CSN, SCK, MOSI, MISO, IRQ. **Owner confirms their two specific modules use the same physical eight-pin header order and orientation.** Pin 3 (CC1101 GDO0 versus nRF CE) changes between input and output, and pin 8 serves different functions. The shared socket is designed for these two confirmed module boards, not arbitrary CC1101/nRF24 breakout models.

## Firmware behaviour
- A single selected radio mode, persisted in preferences. Swapping radios requires power-down and selecting the new mode when back on.
- Shared SPI concept; **hardware probing and RF monitoring remain disabled until the actual connection to M5StickC Plus2 GPIOs is confirmed and electrically safe**.
- No attempt to probe both modules simultaneously, and no GPIO drive based solely on guessing which module is present.
- CC1101 and nRF24 configurations use separate radio-specific control-pin directions.
- No RF transmission from the Radio Lab. Offline metadata inspection is permitted.

## Power and verification
Both modules must have correctly regulated **3.3 V**, never 5 V on the radio VCC. PA/LNA high-power boards may require more supply current than an M5StickC Plus2 pin can safely deliver. Direct connection without extra parts depends on the exact modules, their peak current requirements, and available IO pins.

**Before enabling hardware probing:** map the depicted M5StickC Plus2 GPIO routing into a documented, safe pin configuration and check the 3.3 V current budget. The owner's same-order socket confirmation applies to their actual two boards; it does not validate power delivery or a GPIO that the ESP32 cannot output on. This does not require an SD card, transistor or external controller.

## Additional pinout reference supplied by owner (10 November 2026)
Gemini share: https://share.gemini.google/VPJnJaUOttgy

**Verification status:** The share link could not be read by the build assistant. Its wiring and GPIO values have not been extracted or checked. Do not set `MAMBO_RADIO_HW_VERIFIED` or assign SPI pins from this inaccessible source. A directly uploaded screenshot or the text pin-to-pin mapping is required for electrical validation.
