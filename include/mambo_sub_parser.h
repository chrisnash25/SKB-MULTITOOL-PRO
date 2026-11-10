#pragma once
#include <Arduino.h>
#include <Stream.h>

// Read-only Flipper .sub metadata parser. No RF output or credential replay.
namespace MamboSub {
struct Metadata {
  String filetype;
  String protocol;
  String preset;
  uint32_t frequencyHz = 0;
  bool hasRawData = false;
  bool valid = false;
  String error;
};
inline Metadata parse(Stream &stream, size_t maxBytes = 65536) {
  Metadata m;
  size_t readBytes = 0;
  while (stream.available() && readBytes < maxBytes) {
    String line = stream.readStringUntil('\n');
    readBytes += line.length() + 1;
    line.trim();
    if (line.startsWith("#") || line.isEmpty()) continue;
    int colon = line.indexOf(':');
    if (colon < 1) continue;
    String key = line.substring(0, colon);
    String value = line.substring(colon + 1);
    key.trim(); value.trim();
    if (key == "Filetype") m.filetype = value;
    else if (key == "Protocol") m.protocol = value;
    else if (key == "Preset") m.preset = value;
    else if (key == "RAW_Data") m.hasRawData = true;
    else if (key == "Frequency") {
      if (value.isEmpty()) { m.error = "invalid_frequency"; return m; }
      for (size_t i=0;i<value.length();++i)
        if (!isDigit(value[i])) { m.error = "invalid_frequency"; return m; }
      uint64_t hz = strtoull(value.c_str(), nullptr, 10);
      if (hz == 0 || hz > 0xFFFFFFFFULL) { m.error = "invalid_frequency"; return m; }
      m.frequencyHz = (uint32_t)hz;
    }
  }
  if (readBytes >= maxBytes && stream.available()) m.error = "file_too_large";
  else if (m.filetype != "Flipper SubGhz Key File") m.error = "unsupported_filetype";
  else if (!m.frequencyHz) m.error = "missing_frequency";
  else if (m.protocol.isEmpty()) m.error = "missing_protocol";
  else m.valid = true;
  return m;
}
} // namespace MamboSub
