# Flipper-IRDB integration

Source: https://github.com/Lucaslhm/Flipper-IRDB

The repository contains Flipper Zero .ir profiles grouped by device type and brand. Upstream states CC0-1.0 applies to contributions after commit 2319685; review provenance before redistributing older files.

## Import model
Download selected .ir files to the phone, then upload to LittleFS /ir/. The parser in src/infrared/FlipperIR.cpp reads parsed and raw records with bounds checks. It does not transmit signals by itself.

## Compatibility
Parsed commands need explicit mapping to supported IRremoteESP8266 protocol identifiers; raw timing arrays need a carrier-capable sender on the verified onboard IR TX pin. Unsupported formats must be rejected rather than guessed.

## Status
Parser source committed; not compiled, integrated into the web UI, or hardware-tested. No IRDB files copied into this repository yet.
