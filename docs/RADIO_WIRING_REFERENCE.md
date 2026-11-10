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

## Exact GPIO map from owner-uploaded screenshot (10 November 2026)

The owner supplied an image titled **CC1101 Standalone Pinout Map**. The screenshot specifies these direct electrical connections:

| Shared eight-position socket signal | M5StickC Plus2 / ESP32 GPIO |
|---|---|
| VCC | 3V3 only |
| GND | GND |
| CSN / CS | GPIO26 |
| SCK / CLK | GPIO0 |
| MOSI / SI | GPIO32 |
| MISO / SO | GPIO33 |
| CC1101 GDO0 / nRF24 CE | GPIO25 |
| CC1101 GDO2 / nRF24 IRQ | Not connected (optional) |

The user confirms both particular radio boards have matching 8-pin ordering, and that only one radio is attached at a time.

**ESP32 GPIO0 boot warning:** GPIO0 is a strapping pin, sampled during reset. Using it for SCK can disrupt normal boot or flashing if the connected radio board forces an incompatible level. GPIO0 is an output-capable GPIO after boot, but it is not a universally safe SPI clock pin at reset.

**Board/power warning:** The screenshot shows signal assignments; it does not independently establish that these M5StickC Plus2 pads are all physically available, that GPIO25/32/33 are not used internally by the specific unit, that a PA/LNA radio's peak power is within the board's 3.3V budget, or that GPIO0 is safe with the actual wiring. Do not enable a radio profile based solely on this diagram without these checks.

### Corresponding firmware build flags (not enabled in default builds)

```ini
-D MAMBO_RADIO_PIN_CSN=26
-D MAMBO_RADIO_PIN_SCK=0
-D MAMBO_RADIO_PIN_MOSI=32
-D MAMBO_RADIO_PIN_MISO=33
-D MAMBO_RADIO_PIN_CONTROL=25
```

`MAMBO_RADIO_HW_VERIFIED` is deliberately omitted from the normal build. A dedicated validated build can add it once the power, boot and physical pin checks have been completed. GPIO0 cannot be assumed boot-safe simply because these assignments appear in a screenshot.
