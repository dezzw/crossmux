#include <HalOtaSlot.h>
#include <gtest/gtest.h>

#include "SdCardFontSystem.h"

namespace OtaPostInstallTestHost {
void resetHalOtaSlotHost();
void setInactiveSlotSize(size_t size);
void setRunningPendingVerify();
int confirmCallCount();
std::vector<uint8_t>& inactiveSlotBytes();
extern int releaseLoadedFontCalls;
extern int ensureLoadedCalls;
extern bool lastEnsureAllowFlash;
}  // namespace OtaPostInstallTestHost

namespace {
class GfxRendererStub {};
GfxRendererStub gRenderer;
}  // namespace

TEST(OtaPostInstall, ConfirmsPendingVerifyOnceForAnyHomeRoute) {
  using namespace OtaPostInstallTestHost;
  resetHalOtaSlotHost();
  setRunningPendingVerify();

  EXPECT_EQ(HalOtaSlot::runningImageState(), HalOtaSlot::RunningImageState::PendingVerify);
  EXPECT_TRUE(HalOtaSlot::confirmRunningImage());
  EXPECT_EQ(confirmCallCount(), 1);
  EXPECT_EQ(HalOtaSlot::runningImageState(), HalOtaSlot::RunningImageState::Confirmed);

  EXPECT_TRUE(HalOtaSlot::confirmRunningImage());
  EXPECT_EQ(confirmCallCount(), 1);
}

TEST(OtaPostInstall, NoPreloadScratchWhilePendingVerify) {
  using namespace OtaPostInstallTestHost;
  resetHalOtaSlotHost();
  setInactiveSlotSize(128 * 1024);
  setRunningPendingVerify();

  EXPECT_FALSE(HalOtaSlot::inactive().safeForScratchWrite());
  EXPECT_TRUE(HalOtaSlot::confirmRunningImage());
  EXPECT_TRUE(HalOtaSlot::inactive().safeForScratchWrite());
}

TEST(OtaPostInstall, UnloadsFontsBeforeInactiveSlotWrite) {
  using namespace OtaPostInstallTestHost;
  resetHalOtaSlotHost();
  releaseLoadedFontCalls = 0;
  ensureLoadedCalls = 0;

  sdFontSystem.releaseLoadedFont(reinterpret_cast<GfxRenderer&>(gRenderer));
  EXPECT_EQ(releaseLoadedFontCalls, 1);

  setInactiveSlotSize(64 * 1024);
  inactiveSlotBytes().assign(64 * 1024, 0x5A);
  const uint8_t otaChunk[4096] = {};
  ASSERT_TRUE(HalOtaSlot::inactive().write(0, otaChunk, sizeof(otaChunk)));
  uint8_t readBack[8] = {};
  ASSERT_TRUE(HalOtaSlot::inactive().read(0, readBack, sizeof(readBack)));
  EXPECT_EQ(readBack[0], otaChunk[0]);

  sdFontSystem.ensureLoaded(reinterpret_cast<GfxRenderer&>(gRenderer), false);
  EXPECT_EQ(ensureLoadedCalls, 1);
  EXPECT_FALSE(lastEnsureAllowFlash);
}
