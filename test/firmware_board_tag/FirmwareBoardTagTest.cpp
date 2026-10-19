#include <gtest/gtest.h>

#include <cstring>

#include "src/network/FirmwareBoardTag.h"

TEST(FirmwareBoardTag, ManifestBoardTagMatchesRunningBoardName) {
  EXPECT_TRUE(board_tag::manifestBoardTagMatches("waveshare_epaper_397"));
}

TEST(FirmwareBoardTag, RejectsDifferentManifestBoardTag) {
  EXPECT_FALSE(board_tag::manifestBoardTagMatches("simulator"));
  EXPECT_FALSE(board_tag::manifestBoardTagMatches("waveshare_epaper_397x"));
}

TEST(FirmwareBoardTag, RejectsTrailingCharactersAfterBoardName) {
  EXPECT_FALSE(board_tag::manifestBoardTagMatches("waveshare_epaper_397;"));
}

TEST(FirmwareBoardTag, BoardNameIsNotStandaloneCString) {
  EXPECT_NE(0, strcmp("waveshare_epaper_397", board_tag::boardName()));
  EXPECT_EQ(board_tag::boardNameLen(), strlen("waveshare_epaper_397"));
}
