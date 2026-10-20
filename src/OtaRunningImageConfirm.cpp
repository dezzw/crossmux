#include "OtaRunningImageConfirm.h"

#include <Logging.h>

#include "BootActivityPostOtaPreload.h"

namespace {

HalOtaSlot::RunningImageState queryRunningState(const OtaRunningImageConfirmDeps* testDeps) {
  if (testDeps && testDeps->runningImageState) return testDeps->runningImageState();
  return HalOtaSlot::runningImageState();
}

bool confirmRunning(const OtaRunningImageConfirmDeps* testDeps) {
  if (testDeps && testDeps->confirmRunningImage) return testDeps->confirmRunningImage();
  return HalOtaSlot::confirmRunningImage();
}

void preloadAfterConfirm(GfxRenderer& renderer, const OtaRunningImageConfirmDeps* testDeps) {
  if (testDeps && testDeps->preloadFontCacheIfNeeded) {
    testDeps->preloadFontCacheIfNeeded(renderer);
    return;
  }
  postOtaPreloadFontCacheIfNeeded(renderer);
}

}  // namespace

void otaConfirmPendingRunningImageAtHome(GfxRenderer& renderer, const OtaRunningImageConfirmDeps* testDeps) {
  if (queryRunningState(testDeps) != HalOtaSlot::RunningImageState::PendingVerify) return;

  if (confirmRunning(testDeps)) {
    LOG_INF("OTA", "Running image confirmed after Home UI reached");
    preloadAfterConfirm(renderer, testDeps);
  } else {
    LOG_ERR("OTA", "Running image confirmation failed on Home entry");
  }
}
