#include "OtaRunningImageConfirm.h"

#include <Logging.h>

namespace {

HalOtaSlot::RunningImageState queryRunningState(const OtaRunningImageConfirmDeps* testDeps) {
  if (testDeps && testDeps->runningImageState) return testDeps->runningImageState();
  return HalOtaSlot::runningImageState();
}

bool confirmRunning(const OtaRunningImageConfirmDeps* testDeps) {
  if (testDeps && testDeps->confirmRunningImage) return testDeps->confirmRunningImage();
  return HalOtaSlot::confirmRunningImage();
}

}  // namespace

void otaConfirmPendingRunningImage(const OtaRunningImageConfirmDeps* testDeps) {
  if (queryRunningState(testDeps) != HalOtaSlot::RunningImageState::PendingVerify) return;

  if (confirmRunning(testDeps)) {
    LOG_INF("OTA", "Running image confirmed");
  } else {
    LOG_ERR("OTA", "Running image confirmation failed");
  }
}
