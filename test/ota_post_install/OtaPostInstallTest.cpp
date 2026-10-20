#include <HalOtaSlot.h>
#include <gtest/gtest.h>

#include "OtaRunningImageConfirm.h"

namespace {
class GfxRendererStub {};
GfxRendererStub gRenderer;

HalOtaSlot::RunningImageState gState = HalOtaSlot::RunningImageState::PendingVerify;
int gConfirmCalls = 0;

HalOtaSlot::RunningImageState testRunningState() { return gState; }

bool testConfirm() {
  ++gConfirmCalls;
  gState = HalOtaSlot::RunningImageState::Confirmed;
  return true;
}

void resetHooks() {
  gState = HalOtaSlot::RunningImageState::PendingVerify;
  gConfirmCalls = 0;
}
}  // namespace

TEST(OtaRunningImageConfirm, ConfirmsOnceWhenPending) {
  resetHooks();
  OtaRunningImageConfirmDeps deps{};
  deps.runningImageState = testRunningState;
  deps.confirmRunningImage = testConfirm;

  otaConfirmPendingRunningImageAtHome(reinterpret_cast<GfxRenderer&>(gRenderer), &deps);
  EXPECT_EQ(gConfirmCalls, 1);

  otaConfirmPendingRunningImageAtHome(reinterpret_cast<GfxRenderer&>(gRenderer), &deps);
  EXPECT_EQ(gConfirmCalls, 1);
}
