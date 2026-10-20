#include <HalOtaSlot.h>

HalOtaSlot HalOtaSlot::inactive() { return {}; }

HalOtaSlot::RunningImageState HalOtaSlot::runningImageState() {
  return RunningImageState::Confirmed;
}

bool HalOtaSlot::confirmRunningImage() { return true; }

bool HalOtaSlot::safeForScratchWrite() const { return false; }

bool HalOtaSlot::read(size_t, void*, size_t) const { return false; }

bool HalOtaSlot::erase(size_t, size_t) const { return false; }

bool HalOtaSlot::write(size_t, const void*, size_t) const { return false; }

bool HalOtaSlot::contains(size_t, size_t) const { return false; }
