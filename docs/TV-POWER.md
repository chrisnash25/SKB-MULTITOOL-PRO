# TV / monitor power controls

Use only on selected equipment that you own or are authorised to control.

## Behaviour
Power is a toggle: if a display is off, the same command may switch it on. Prefer discrete Power_off when available. Never blast every IRDB code at nearby equipment.

## Status
PowerToggle.h is an initial IRsend adapter, not an integrated firmware feature. It has not been compiled or hardware-tested. Flipper .ir files use little-endian spaced hex bytes; the existing parser's strtoull conversion is not correct for those values and must be fixed before transmission. No dashboard endpoint or UI button is wired yet.

## Upstream
https://github.com/Lucaslhm/Flipper-IRDB
https://developer.flipper.net/flipperzero/doxygen/infrared_file_format.html
