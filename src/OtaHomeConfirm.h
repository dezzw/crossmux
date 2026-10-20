#pragma once

#include <HalOtaSlot.h>

namespace OtaHomeConfirm {

struct Outcome {
  HalOtaSlot::RunningImageState stateBefore = HalOtaSlot::RunningImageState::Unsafe;
  bool attemptedConfirm = false;
  bool confirmSucceeded = false;
};

// Idempotent: confirms a pending-verify running image once; no-op otherwise.
Outcome confirmPendingIfNeeded();

}  // namespace OtaHomeConfirm
