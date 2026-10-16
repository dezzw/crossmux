#include <gtest/gtest.h>

#include "lib/JsonParser/SemVerCompare.h"

TEST(SemVerCompare, NewerPatch) { EXPECT_EQ(semverCompareNewer("1.0.0", "1.0.1"), 1); }

TEST(SemVerCompare, RejectsDowngrade) { EXPECT_EQ(semverCompareNewer("1.2.0", "1.1.9"), -1); }

TEST(SemVerCompare, EqualStable) { EXPECT_EQ(semverCompareNewer("2.0.0", "2.0.0"), 0); }

TEST(SemVerCompare, RcCurrentStableLatest) { EXPECT_EQ(semverCompareNewer("1.0.0-rc", "1.0.0"), 1); }

TEST(SemVerCompare, IgnoresBuildMetadataForUpgradeDecision) {
  EXPECT_EQ(semverCompareNewer("1.0.0-waveshare-epaper-397-rc.1+deadbeef", "1.0.1"), 1);
  EXPECT_EQ(semverCompareNewer("1.0.0-waveshare-epaper-397-rc.1", "1.0.0-waveshare-epaper-397-rc.1+aaaa"), 0);
}

TEST(SemVerCompare, NightlyRcRunOrdering) {
  EXPECT_EQ(semverCompareNewer("1.6.0-waveshare-epaper-397-rc.100", "1.6.0-waveshare-epaper-397-rc.101"), 1);
  EXPECT_EQ(semverCompareNewer("1.6.0-waveshare-epaper-397-rc.101", "1.6.0-waveshare-epaper-397-rc.100"), -1);
  EXPECT_EQ(semverCompareNewer("1.6.0-waveshare-epaper-397-rc.9", "1.6.0-waveshare-epaper-397-rc.10"), 1);
}
