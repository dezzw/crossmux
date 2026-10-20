#include <gtest/gtest.h>

#include <HalOtaSlot.h>
#include "OtaHomeConfirm.h"
#include "OtaInstallFontGuard.h"

namespace OtaPostInstallTestHost {
void resetHalOtaSlotHost();
void setInactiveSlotSize(size_t size);
void setRunningPendingVerify();
void setRunningConfirmed();
int confirmCallCount();
std::vector<uint8_t>& inactiveSlotBytes();
void overwriteInactiveSlot(size_t offset, const void* data, size_t length);
extern int releaseLoadedFontCalls;
extern int ensureLoadedCalls;
extern bool lastEnsureAllowFlash;
}  // namespace OtaPostInstallTestHost

namespace {
class GfxRendererStub {};
GfxRendererStub gRenderer;
}  // namespace

TEST(OtaHomeConfirm, ConfirmsPendingVerifyOnceForAnyHomeRoute) {
  using namespace OtaPostInstallTestHost;
  resetHalOtaSlotHost();
  setRunningPendingVerify();

  const auto first = OtaHomeConfirm::confirmPendingIfNeeded();
  EXPECT_TRUE(first.attemptedConfirm);
  EXPECT_TRUE(first.confirmSucceeded);
  EXPECT_EQ(confirmCallCount(), 1);
  EXPECT_EQ(HalOtaSlot::runningImageState(), HalOtaSlot::RunningImageState::Confirmed);

  const auto second = OtaHomeConfirm::confirmPendingIfNeeded();
  EXPECT_FALSE(second.attemptedConfirm);
  EXPECT_TRUE(second.confirmSucceeded);
  EXPECT_EQ(confirmCallCount(), 1);
}

TEST(OtaHomeConfirm, NoPreloadScratchWhilePendingVerify) {
  using namespace OtaPostInstallTestHost;
  resetHalOtaSlotHost();
  setInactiveSlotSize(128 * 1024);
  setRunningPendingVerify();

  EXPECT_EQ(HalOtaSlot::runningImageState(), HalOtaSlot::RunningImageState::PendingVerify);
  EXPECT_FALSE(HalOtaSlot::inactive().safeForScratchWrite());

  OtaHomeConfirm::confirmPendingIfNeeded();
  EXPECT_EQ(HalOtaSlot::runningImageState(), HalOtaSlot::RunningImageState::Confirmed);
  EXPECT_TRUE(HalOtaSlot::inactive().safeForScratchWrite());
}

TEST(OtaInstallFontGuard, UnloadsFontsBeforeInactiveSlotWrite) {
  using namespace OtaPostInstallTestHost;
  resetHalOtaSlotHost();
  releaseLoadedFontCalls = 0;
  ensureLoadedCalls = 0;

  OtaInstallFontGuard::beforeInactiveSlotWrite(reinterpret_cast<GfxRenderer&>(gRenderer));
  EXPECT_EQ(releaseLoadedFontCalls, 1);

  setInactiveSlotSize(64 * 1024);
  inactiveSlotBytes().assign(64 * 1024, 0x5A);
  const uint8_t otaChunk[4096] = {};
  ASSERT_TRUE(HalOtaSlot::inactive().write(0, otaChunk, sizeof(otaChunk)));
  uint8_t readBack[8] = {};
  ASSERT_TRUE(HalOtaSlot::inactive().read(0, readBack, sizeof(readBack)));
  EXPECT_EQ(readBack[0], otaChunk[0]);

  OtaInstallFontGuard::reloadWithoutFlashCache(reinterpret_cast<GfxRenderer&>(gRenderer));
  EXPECT_EQ(ensureLoadedCalls, 1);
  EXPECT_FALSE(lastEnsureAllowFlash);
}
