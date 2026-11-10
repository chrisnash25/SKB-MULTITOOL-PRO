#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
json_dir="${ARDUINOJSON_DIR:-.pio/libdeps/m5stick-cplus2/ArduinoJson}"
if [[ ! -f "$json_dir/src/ArduinoJson.h" ]]; then
  echo 'Set ARDUINOJSON_DIR to an ArduinoJson 7.4.2 checkout, or install PlatformIO dependencies first.' >&2
  exit 1
fi
binary=$(mktemp /tmp/skb-wifi-test.XXXXXX)
trap 'rm -f "$binary"' EXIT
g++ -std=c++11 -Wall -Wextra -Werror \
  -DARDUINOJSON_ENABLE_ARDUINO_STRING=1 \
  -DARDUINOJSON_ENABLE_ARDUINO_STREAM=0 \
  -DARDUINOJSON_ENABLE_ARDUINO_PRINT=0 \
  -DARDUINOJSON_ENABLE_PROGMEM=0 \
  -Itests/stubs -Isrc -I"$json_dir/src" \
  src/wifi/WifiDiagnostics.cpp tests/test_wifi.cpp -o "$binary"
"$binary"
node tests/test-dashboard.cjs
