#pragma once
#include <string>
#include <cstdint>
#include <cstdio>
class String {
  std::string value_;
 public:
  String() = default;
  String(const char* value):value_(value ? value : "") {}
  const char* c_str() const {return value_.c_str();}
  unsigned int length() const {return value_.size();}
  char operator[](size_t i) const {return value_[i];}
  bool concat(const char* text) {value_+=text; return true;}
  bool concat(const char* text, size_t n) {value_.append(text,n);return true;}
  bool operator==(const String& other) const {return value_==other.value_;}
  bool operator!=(const String& other) const {return !(*this==other);}
};
extern uint32_t testNow;
inline uint32_t millis() {return testNow;}
