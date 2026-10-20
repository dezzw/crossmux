#include <HalOtaSlot.h>
#include <gtest/gtest.h>

#include "OtaRunningImageConfirm.h"
#include "OtaUpdateInstallPrep.h"

namespace OtaPostInstallTestHost {
extern int releaseLoadedFontCalls;
extern int preloadCalls;
}  // namespace OtaPostInstallTestHost

namespace {
class GfxRendererStub {};
GfxRendererStub gRenderer;

HalOtaSlot::RunningImageState gState = HalOtaSlot::RunningImageState::PendingVerify;
int gConfirmCalls = 0;
int gPreloadCalls = 0;
int gReleaseCalls = 0;

HalOtaSlot::RunningImageState testRunningState() { return gState; }

bool testConfirm() {
  ++gConfirmCalls;
  gState = HalOtaSlot::RunningImageState::Confirmed;
  return true;
}

void testPreload(GfxRenderer&) { ++gPreloadCalls; }

void testRelease(GfxRenderer&) { ++gReleaseCalls; }

void resetHooks() {
  gState = HalOtaSlot::RunningImageState::PendingVerify;
  gConfirmCalls = 0;
  gPreloadCalls = 0;
  gReleaseCalls = 0;
}
}  // namespace

TEST(OtaRunningImageConfirm, ConfirmsAndPreloadsOnceWhenPending) {
  resetHooks();
  OtaRunningImageConfirmDeps deps{};
  deps.runningImageState = testRunningState;
  deps.confirmRunningImage = testConfirm;
  deps.preloadFontCacheIfNeeded = testPreload;

  using namespace OtaPostInstallTestHost;
  preloadCalls = 0;

  otaConfirmPendingRunningImageAtHome(reinterpret_cast<GfxRenderer&>(gRenderer), &deps);
  EXPECT_EQ(gConfirmCalls, 1);
  EXPECT_EQ(gPreloadCalls, 1);
  EXPECT_EQ(preloadCalls, 0);

  otaConfirmPendingRunningImageAtHome(reinterpret_cast<GfxRenderer&>(gRenderer), &deps);
  EXPECT_EQ(gConfirmCalls, 1);
  EXPECT_EQ(gPreloadCalls, 1);
}

TEST(OtaRunningImageConfirm, DefaultPreloadUsesProductionHookWhenNoDeps) {
  using namespace OtaPostInstallTestHost;
  resetHooks();
  preloadCalls = 0;
  OtaRunningImageConfirmDeps deps{};
  deps.runningImageState = testRunningState;
  deps.confirmRunningImage = testConfirm;
  deps.preloadFontCacheIfNeeded = nullptr;

  otaConfirmPendingRunningImageAtHome(reinterpret_cast<GfxRenderer&>(gRenderer), &deps);
  EXPECT_EQ(preloadCalls, 1);
}

TEST(OtaUpdateInstallPrep, ReleaseLoadedFontBeforeInactiveSlotWrite) {
  using namespace OtaPostInstallTestHost;
  releaseLoadedFontCalls = 0;
  otaReleaseFontsBeforeInactiveSlotWrite(reinterpret_cast<GfxRenderer&>(gRenderer), nullptr);
  EXPECT_EQ(releaseLoadedFontCalls, 1);

  gReleaseCalls = 0;
  OtaUpdateInstallPrepDeps deps{};
  deps.releaseLoadedFont = testRelease;
  otaReleaseFontsBeforeInactiveSlotWrite(reinterpret_cast<GfxRenderer&>(gRenderer), &deps);
  EXPECT_EQ(gReleaseCalls, 1);
}
