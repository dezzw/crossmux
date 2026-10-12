#include <gtest/gtest.h>

#include "components/themes/ThemeDevicePolicy.h"

TEST(ThemeDevicePolicy, KeyInstructionTipsVisibility) {
#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
  EXPECT_FALSE(deviceKeyInstructionTipsVisible());
#else
  EXPECT_TRUE(deviceKeyInstructionTipsVisible());
#endif
}

TEST(ThemeDevicePolicy, ButtonHintsVisibility) {
#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
  EXPECT_FALSE(deviceButtonHintsVisible(true));
  EXPECT_FALSE(deviceButtonHintsVisible(false));
#else
  EXPECT_TRUE(deviceButtonHintsVisible(true));
  EXPECT_FALSE(deviceButtonHintsVisible(false));
#endif
}

TEST(ThemeDevicePolicy, ZeroButtonHintMetricsWhenHintsHidden) {
  int buttonHintsHeight = 40;
  int sideButtonHintsWidth = 30;
  deviceApplyButtonHintMetrics(buttonHintsHeight, sideButtonHintsWidth, false);
  EXPECT_EQ(buttonHintsHeight, 0);
  EXPECT_EQ(sideButtonHintsWidth, 0);

  buttonHintsHeight = 40;
  sideButtonHintsWidth = 30;
  deviceApplyButtonHintMetrics(buttonHintsHeight, sideButtonHintsWidth, true);
  EXPECT_EQ(buttonHintsHeight, 40);
  EXPECT_EQ(sideButtonHintsWidth, 30);
}

#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
TEST(ThemeDevicePolicy, WaveshareMetricsZeroedWithDevicePolicy) {
  int buttonHintsHeight = 40;
  int sideButtonHintsWidth = 30;
  deviceApplyButtonHintMetrics(buttonHintsHeight, sideButtonHintsWidth, deviceButtonHintsVisible(true));
  EXPECT_EQ(buttonHintsHeight, 0);
  EXPECT_EQ(sideButtonHintsWidth, 0);
}
#endif
