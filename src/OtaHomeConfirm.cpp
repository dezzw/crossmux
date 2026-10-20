#include "OtaHomeConfirm.h"

namespace OtaHomeConfirm {

Outcome confirmPendingIfNeeded() {
  Outcome outcome{};
  outcome.stateBefore = HalOtaSlot::runningImageState();
  if (outcome.stateBefore != HalOtaSlot::RunningImageState::PendingVerify) {
    outcome.confirmSucceeded =
        outcome.stateBefore == HalOtaSlot::RunningImageState::Confirmed ||
        outcome.stateBefore == HalOtaSlot::RunningImageState::Untracked;
    return outcome;
  }

  outcome.attemptedConfirm = true;
  outcome.confirmSucceeded = HalOtaSlot::confirmRunningImage();
  return outcome;
}

}  // namespace OtaHomeConfirm
