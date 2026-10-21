#pragma once

#include <HalOtaSlot.h>

struct OtaRunningImageConfirmDeps {
  HalOtaSlot::RunningImageState (*runningImageState)() = nullptr;
  bool (*confirmRunningImage)() = nullptr;
};

// Idempotent: confirms a pending-verify running image once UI/storage are ready.
void otaConfirmPendingRunningImage(const OtaRunningImageConfirmDeps* testDeps = nullptr);
