#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "src/network/CrossMuxEndpoints.h"

TEST(CrossMuxEndpoints, FormatFontGithubManifestUrl) {
  char url[256];
  const int len = CrossMuxEndpoints::formatFontGithubManifestUrl(url, sizeof(url), 1, 4);
  ASSERT_GT(len, 0);
  EXPECT_LT(static_cast<size_t>(len), sizeof(url));
  EXPECT_STREQ(url,
               "https://github.com/crosspoint-reader/crosspoint-fonts/releases/download/sd-fonts-m1-b4/fonts.json");
}

TEST(CrossMuxEndpoints, FormatFontLegacyManifestUrl) {
  char url[256];
  const int len = CrossMuxEndpoints::formatFontLegacyManifestUrl(url, sizeof(url), 1, 4);
  ASSERT_GT(len, 0);
  EXPECT_LT(static_cast<size_t>(len), sizeof(url));
  EXPECT_STREQ(url, "https://crossmux.com/api/assets/fonts/m1-b4/fonts.json");
}
