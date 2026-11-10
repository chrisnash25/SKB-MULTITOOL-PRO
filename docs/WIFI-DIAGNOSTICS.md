# Wi-Fi diagnostics and local configuration

M5StickC Plus2 uses its original ESP32 2.4 GHz radio. No 5 GHz support is implied.
The existing AP, IR/NEC, BLE/PathShield routes and serial `status` response remain.
New diagnostics share a modular C++ implementation for HTTP and USB.

## USB OTG contract

Use the existing USB-UART connection at **115200 baud**, newline-delimited JSON.
Each request produces one JSON response line. Existing `{"cmd":"status"}` and
unknown-command/invalid-JSON responses retain their format. No unsolicited scan
results are emitted. A companion must accumulate incoming bytes until newline;
responses can exceed a USB read buffer and should not be limited to 512 bytes.
The existing request limit is 512 bytes; send short requests individually.

| USB command | HTTP equivalent | Behaviour |
| --- | --- | --- |
| `wifi.scan` | POST `/api/wifi/scan` | Start one asynchronous passive scan; reject overlapping scans |
| `wifi.results` | GET `/api/wifi/results` | State and last completed network snapshot |
| `wifi.signals` | GET `/api/signal/wifi` | Existing signals shape plus scan metadata |
| `wifi.channels` | GET `/api/wifi/channels` | AP count and RSSI overlap score per locally permitted channel |
| `wifi.devices` | GET `/api/wifi/devices` | Bounded AP observation history and clients of this device's own AP |
| `config.get` | GET `/api/config` | Device label and passive scan dwell |
| `config.set` | POST `/api/config` | Persist validated label and scan dwell |

Example USB sequence:

```json
{"cmd":"wifi.scan"}
{"cmd":"wifi.results"}
{"cmd":"wifi.channels"}
{"cmd":"wifi.devices"}
{"cmd":"config.set","label":"Kitchen Stick","scan_dwell_ms":120}
```

Poll `wifi.results` about once per second while `state` is `scanning`. Terminal
states are `complete`, `failed`, `timeout`; initial state is `idle`. HTTP scan
start returns 202, overlap returns 409, start failure returns 503. USB uses the
same `ok` and `error` fields without HTTP status codes. Result reads always
return HTTP 200; inspect `ok` and `state` for completion errors.

`generation`, `has_results`, `completed_uptime_ms`, and `age_ms` identify the
last completed scan. Results survive repeated reads and subsequent failures.
An empty completed scan replaces the snapshot with an empty list. Up to 64 APs
are retained; `truncated` reports a scan beyond that limit. History is RAM-only,
bounded to 64 BSSIDs and evicts old entries. `seen_last_scan` means observed in
the last completed scan; it does not indicate continuous presence. Uptime/age
use unsigned millisecond arithmetic and wrap after roughly 49.7 days.

The channel score sums `(clamp(RSSI + 100, 1, 100) * (5 - channel_distance))`
for channel distances 0 through 4. It estimates relative overlap from observed
APs, not airtime, noise floor, throughput or spectrum occupancy. A suggested
channel is the lowest-scored permitted member of 1/6/11, only after a scan.
The AP itself and hidden networks may affect observations. Passive scanning
sends no probe requests. There is no arbitrary client sniffing, payload capture,
connection probing, deauthentication or network impersonation.

## Local portal

Join `SKB-MULTITOOL-PRO` and open `http://192.168.4.1/`. Wildcard DNS and
non-API HTTP redirects support captive portal discovery. Unknown `/api/` paths
still return JSON 404. HTTPS interception is not supported; Android may require
choosing to stay connected to a network without Internet and opening the address
manually. USB OTG remains serial; it does not provide an IP network interface.

The dashboard and `data/index.html` contain the same branded, offline UI.
No external sign-in page or credential form is served. Settings are a label
(1–32 UTF-8 bytes, no ASCII controls) and passive dwell (100–500 ms per channel).
Settings do not rename/restart the AP or change USB framing. POST settings need
JSON and reject a foreign browser Origin. This is not authentication: the
existing AP remains open and attached clients can read diagnostics and change
these non-secret settings. Operate it in a trusted local environment.

## Android integration status

No Android/Gradle project or `build-usb-android.yml` exists in the inspected
`main` commit. These commands are ready for integration into Mambo OS UI once
its source is supplied. The existing web dashboard has been updated for phone
use; no native Android code or APK was built or validated.

## Validation

Run `pio run -e m5stick-cplus2 -j 4`, then `tests/run-host-tests.sh`, and
`pio run -e m5stick-cplus2 -t buildfs`. Host tests use ArduinoJson 7.4.2 from
PlatformIO's installed dependencies. Alternatively set `ARDUINOJSON_DIR` to a
checkout of that version. Host tests compile the production diagnostics module
with simulated radio, clock and preferences APIs; they do not validate ESP32
SDK linkage or real radio operation. Dashboard tests execute the actual embedded
JavaScript with mocked HTTP and DOM APIs.

Hardware validation still required:

- Flash on Plus2 and verify display, power, USB `status`, NEC IR and BLE routes.
- On S21 OTG, verify fragmented/long JSON response assembly and existing commands.
- Compare passive results to known 2.4 GHz APs and verify empty/repeated scans.
- Test AP client join/leave reporting, portal DNS/redirects and settings after reboot.
- Exercise USB, web requests and BLE during Wi-Fi scans; existing BLE discovery
  blocks for three seconds, and off-channel Wi-Fi scanning can delay AP traffic.
- Check free heap under repeated scans with 64 AP observations and BLE activity.
