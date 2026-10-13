#include <gtest/gtest.h>

#include "lib/JsonParser/OtaManifestParser.h"

TEST(OtaManifestParser, SignedDeviceManifest) {
  const char* json = R"({
    "version":"1.6.0",
    "boardTag":"waveshare_epaper_397",
    "signature":"AQID",
    "firmware":{
      "url":"https://github.com/dezzw/crossmux/releases/download/1.6.0/waveshare-epaper-397-firmware.bin",
      "size":100,
      "sha256":"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
      "signature":"AQID"
    }
  })";
  OtaManifestParser parser;
  parser.feed(json, strlen(json));
  EXPECT_TRUE(parser.foundVersion());
  EXPECT_TRUE(parser.foundBoardTag());
  EXPECT_TRUE(parser.foundFirmware());
  EXPECT_TRUE(parser.foundSignature());
  EXPECT_STREQ(parser.getVersion(), "1.6.0");
  EXPECT_STREQ(parser.getBoardTag(), "waveshare_epaper_397");
  EXPECT_EQ(parser.getFirmwareSize(), 100u);
}
