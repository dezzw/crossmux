#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>

#include "EpdFont/SdCardFont.h"
#include "src/network/CrossMuxEndpoints.h"

// Keep aligned with FONTS_MANIFEST_VERSION in FontDownloadActivity.h.
constexpr int kFontsManifestVersion = 1;

TEST(CrossMuxEndpoints, FormatFontGithubManifestUrlUsesBuildConstants) {
  char url[256];
  const int len =
      CrossMuxEndpoints::formatFontGithubManifestUrl(url, sizeof(url), kFontsManifestVersion, CPFONT_VERSION);
  ASSERT_GT(len, 0);
  EXPECT_LT(static_cast<size_t>(len), sizeof(url));

  char expected[256];
  snprintf(expected, sizeof(expected),
           "https://github.com/crosspoint-reader/crosspoint-fonts/releases/download/sd-fonts-m%d-b%d/fonts.json",
           kFontsManifestVersion, CPFONT_VERSION);
  EXPECT_STREQ(url, expected);
}

TEST(CrossMuxEndpoints, FormatFontLegacyManifestUrlUsesBuildConstants) {
  char url[256];
  const int len =
      CrossMuxEndpoints::formatFontLegacyManifestUrl(url, sizeof(url), kFontsManifestVersion, CPFONT_VERSION);
  ASSERT_GT(len, 0);
  EXPECT_LT(static_cast<size_t>(len), sizeof(url));

  char expected[256];
  snprintf(expected, sizeof(expected), "https://crossmux.com/api/assets/fonts/m%d-b%d/fonts.json",
           kFontsManifestVersion, CPFONT_VERSION);
  EXPECT_STREQ(url, expected);
}

TEST(CrossMuxEndpoints, FormatFontGithubManifestUrlTruncatesWhenBufferTooSmall) {
  char url[8];
  const int len =
      CrossMuxEndpoints::formatFontGithubManifestUrl(url, sizeof(url), kFontsManifestVersion, CPFONT_VERSION);
  EXPECT_LT(len, 0);
}
