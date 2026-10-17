#pragma once

#include <cstdint>

#include "Client.h"

using byte = uint8_t;

inline unsigned long gMockMillis = 0;
inline unsigned long millis() { return gMockMillis; }
inline void delay(unsigned long) {}

struct SerialStub {
  operator bool() const { return false; }
  void println(const char*) const {}
};
inline SerialStub Serial;
