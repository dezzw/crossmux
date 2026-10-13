#include <gtest/gtest.h>

#include "lib/JsonParser/SemVerCompare.h"

TEST(SemVerCompare, NewerPatch) { EXPECT_EQ(semverCompareNewer("1.0.0", "1.0.1"), 1); }

TEST(SemVerCompare, RejectsDowngrade) { EXPECT_EQ(semverCompareNewer("1.2.0", "1.1.9"), -1); }

TEST(SemVerCompare, EqualStable) { EXPECT_EQ(semverCompareNewer("2.0.0", "2.0.0"), 0); }

TEST(SemVerCompare, RcCurrentStableLatest) { EXPECT_EQ(semverCompareNewer("1.0.0-rc", "1.0.0"), 1); }

TEST(SemVerCompare, IgnoresNightlySuffix) {
  EXPECT_EQ(semverCompareNewer("1.0.0-waveshare-epaper-397-rc+deadbeef", "1.0.1"), 1);
}

TEST(SemVerCompare, NightlyBuildShaUpdate) {
  EXPECT_EQ(semverCompareNewer("1.6.0-waveshare-epaper-397-rc+aaaaaaa", "1.6.0-waveshare-epaper-397-rc+bbbbbbb"), 1);
  EXPECT_EQ(semverCompareNewer("1.6.0-waveshare-epaper-397-rc+bbbbbbb", "1.6.0-waveshare-epaper-397-rc+aaaaaaa"), -1);
}
