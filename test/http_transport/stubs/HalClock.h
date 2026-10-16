#pragma once

#include <ctime>

enum class ClockSyncState : uint8_t { Idle, Syncing, Succeeded, Failed };

class HalClock {
 public:
  mutable time_t now = 0;
  bool syncNowResult = false;

  time_t nowUtc() const { return now; }
  bool hasValidTime() const { return now != 0; }
  bool syncNow(uint32_t = 10000) {
    if (syncNowResult) now = 1'704'016'800;
    return syncNowResult;
  }
  ClockSyncState syncState() const { return ClockSyncState::Idle; }
};

inline HalClock halClock;
