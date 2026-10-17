#pragma once

#include <ctime>

class HalClock {
 public:
  mutable time_t now = 1;

  time_t nowUtc() const { return now; }
  bool hasValidTime() const { return now != 0; }
  bool syncNow(uint32_t = 10000) const { return true; }
};

inline HalClock halClock;
