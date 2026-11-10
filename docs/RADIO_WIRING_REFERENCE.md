# Radio wiring reference — user-supplied M5StickC Plus2 diagram

The supplied CC1101 diagram labels the module header as:
1 GND, 2 VCC, 3 GDO0, 4 CSN, 5 SCK, 6 MOSI, 7 MISO, 8 GDO2.

The pictured M5StickC Plus2 exposes labels GND, 5V, G26, G36/G25, G0, BAT, 3V3, 5V; it also shows a BC547 transistor and a 5 kOhm resistor. This is **not** a validated complete dual-radio wiring diagram.

CC1101 and nRF24L01+ share the SPI signals SCK, MOSI and MISO but **not identical 8-pin control pinouts**. nRF24L01+ requires CSN and CE; IRQ is optional. CC1101 uses CSN, GDO0 and optional GDO2.

Never connect either radio chip directly to 5 V VCC. Verify PA/LNA module board specifications and provide an adequate regulated 3.3 V supply. The diagram's transistor/5 V connections must not be assumed safe or equivalent for nRF24.

Pin mapping remains UNVERIFIED. Do not activate `MamboRadio::Monitor::begin` until an explicit, checked mapping for shared SCK/MISO/MOSI, separate CC1101 CSN, separate nRF24 CSN and CE, and power is available. Consider a GPIO expander or alternative interface where exposed GPIO is insufficient.

For CC1101, frequency monitoring and packet diagnostics are separate from transmitting. For rolling-code systems, support only offline/synthetic format diagnostics and authorized bench testing.
