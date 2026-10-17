#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>

#include "EpdFont/SdCardFont.h"
#include "src/network/CrossMuxEndpoints.h"

constexpr int kFontsManifestVersion = 1;

TEST(CrossMuxEndpoints, FormatFontGithubManifestUrlUsesBuildConstants) {
  char url[256];
  const int len =
      CrossMuxEndpoints::formatFontGithubManifestUrl(url, sizeof(url), kFontsManifestVersion, CPFONT_VERSION);
  ASSERT_GT(len, 0);
  EXPECT_LT(static_cast<size_t>(len), sizeof(url));

  char expected[256];
  snprintf(expected, sizeof(expected), "https://github.com/dezzw/crossmux/releases/download/fonts-m%d-b%d/fonts.json",
           kFontsManifestVersion, CPFONT_VERSION);
  EXPECT_STREQ(url, expected);
}

TEST(CrossMuxEndpoints, FormatOtaStableManifestUrlUsesStableTag) {
  char url[256];
  const int len = CrossMuxEndpoints::formatOtaStableManifestUrl(url, sizeof(url));
  ASSERT_GT(len, 0);
  EXPECT_LT(static_cast<size_t>(len), sizeof(url));
  EXPECT_STREQ(url,
               "https://github.com/dezzw/crossmux/releases/download/stable/waveshare-epaper-397-global-manifest.json");
}

TEST(CrossMuxEndpoints, FormatFontGithubManifestUrlTruncatesWhenBufferTooSmall) {
  char url[8];
  const int len =
      CrossMuxEndpoints::formatFontGithubManifestUrl(url, sizeof(url), kFontsManifestVersion, CPFONT_VERSION);
  EXPECT_GE(len, static_cast<int>(sizeof(url)));
}
