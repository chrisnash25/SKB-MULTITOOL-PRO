# SKB MULTITOOL PRO

M5StickC Plus2 firmware with a phone-first local web dashboard and USB-UART JSON
control. Includes passive Wi-Fi scans, channel overlap analysis, AP observations,
local AP client monitoring and persistent device configuration.

## Development

Use Python 3 and PlatformIO 6.1.18:

```sh
python3 -m venv /workspace/.venv-platformio
/workspace/.venv-platformio/bin/pip install platformio==6.1.18
export PLATFORMIO_CORE_DIR=/workspace/.platformio
/workspace/.venv-platformio/bin/pio run -e m5stick-cplus2 -j 4
tests/run-host-tests.sh
/workspace/.venv-platformio/bin/pio run -e m5stick-cplus2 -t buildfs
```

CI runs on pull requests, main pushes and manual dispatch. See
[Wi-Fi diagnostics](docs/WIFI-DIAGNOSTICS.md) for commands, behaviour and hardware
checks. No Android application source is included in this checkout.

## Hardware and status

Original ESP32; 135x240 TFT; onboard IR TX; USB-UART at 115200 baud.
No assumption of PSRAM, SD, IR receiver or native USB HID.
Host diagnostics and dashboard tests pass. The current cloud environment blocks
PlatformIO registry access, so the ESP32 build and filesystem image have not
been verified here. Hardware and Galaxy S21 USB OTG validation remain pending.

## Local management

Join `SKB-MULTITOOL-PRO` and open `http://192.168.4.1/`. The management AP is
open; use a trusted local environment. The portal only changes non-secret device
settings. No credential collection, deauthentication transmission, jamming or
unauthorised impersonation is implemented.
