#pragma once

#include <cstdint>

// Per-frame budget for drawPixel() out-of-bounds logging (hot path).
class GfxOutsideRangeLogBudget {
 public:
  void resetFrame() {
    loggedThisFrame_ = false;
    suppressedThisFrame_ = 0;
  }

  // True on the first out-of-range pixel in a frame (caller should LOG_ERR).
  bool noteOutsideRange() {
    if (!loggedThisFrame_) {
      loggedThisFrame_ = true;
      return true;
    }
    suppressedThisFrame_++;
    return false;
  }

  uint32_t suppressedThisFrame() const { return suppressedThisFrame_; }

 private:
  bool loggedThisFrame_ = false;
  uint32_t suppressedThisFrame_ = 0;
};
