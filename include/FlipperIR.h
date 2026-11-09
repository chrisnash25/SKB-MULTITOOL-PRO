#pragma once
#include <Arduino.h>
#include <FS.h>
#include <vector>
struct IRCommand {
 String name, protocol;
 uint64_t address=0, command=0;
 uint32_t frequency=38000;
 std::vector<uint16_t> timings;
 bool raw=false;
};
bool parseFlipperIR(fs::FS& fs, const String& path, std::vector<IRCommand>& out, String& error);
