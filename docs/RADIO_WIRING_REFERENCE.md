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
GND, VCC 3.3 V, CE, CSN, SCK, MOSI, MISO, IRQ. The **physical header order and orientation depend on the actual breakout**. Pin 3 (CC1101 GDO0 versus nRF CE) changes between input and output, and optional pin 8 serves different functions. Do not assume identical PCB pin orders from the fact that both have eight pins.

## Firmware behaviour
- A single selected radio mode, persisted in preferences. Swapping radios requires power-down and selecting the new mode when back on.
- Shared SPI concept; **hardware probing and RF monitoring remain disabled until the actual connection to M5StickC Plus2 GPIOs is confirmed and electrically safe**.
- No attempt to probe both modules simultaneously, and no GPIO drive based solely on guessing which module is present.
- CC1101 and nRF24 configurations use separate radio-specific control-pin directions.
- No RF transmission from the Radio Lab. Offline metadata inspection is permitted.

## Power and verification
Both modules must have correctly regulated **3.3 V**, never 5 V on the radio VCC. PA/LNA high-power boards may require more supply current than an M5StickC Plus2 pin can safely deliver. Direct connection without extra parts depends on the exact modules, their peak current requirements, and available IO pins.

**Before enabling hardware probing:** verify the individual module silkscreen/orientation, pin routing from the M5StickC Plus2, and 3.3 V current budget. This does not require an SD card, transistor or external controller.
