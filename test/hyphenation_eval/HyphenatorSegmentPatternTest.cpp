#include <gtest/gtest.h>

#include <algorithm>
#include <utility>
#include <vector>

#include "lib/Epub/Epub/hyphenation/Hyphenator.h"
#include "lib/Epub/Epub/hyphenation/LanguageRegistry.h"

namespace {

bool containsBreak(const std::vector<Hyphenator::BreakInfo>& breaks, const size_t byteOffset,
                   const bool requiresInsertedHyphen) {
  return std::any_of(breaks.begin(), breaks.end(), [&](const Hyphenator::BreakInfo& info) {
    return info.byteOffset == byteOffset && info.requiresInsertedHyphen == requiresInsertedHyphen;
  });
}

}  // namespace

TEST(HyphenatorSegmentPattern, EnglishHyphenatorRunsOnExplicitHyphenSegments) {
  Hyphenator::setPreferredLanguage("en");
  ASSERT_NE(getLanguageHyphenatorForPrimaryTag("en"), nullptr);

  const auto breaks = Hyphenator::breakOffsets("US-abcdefghijklmnop", false);
  EXPECT_TRUE(containsBreak(breaks, 3u, false));
  EXPECT_TRUE(containsBreak(breaks, 8u, true));
}

TEST(HyphenatorSegmentPattern, EnglishHyphenatorRunsOnApostropheSegments) {
  Hyphenator::setPreferredLanguage("en");
  ASSERT_NE(getLanguageHyphenatorForPrimaryTag("en"), nullptr);

  const auto breaks = Hyphenator::breakOffsets("quel'abcdefghijklmnop", false);
  EXPECT_TRUE(containsBreak(breaks, 5u, false));
  EXPECT_TRUE(containsBreak(breaks, 10u, true));
}

TEST(HyphenatorSegmentPattern, EnglishLiangPatternsOnPlainWord) {
  Hyphenator::setPreferredLanguage("en");
  ASSERT_NE(getLanguageHyphenatorForPrimaryTag("en"), nullptr);

  const auto breaks = Hyphenator::breakOffsets("hyphenation", false);
  ASSERT_EQ(breaks.size(), 1u);
  EXPECT_EQ(breaks[0].byteOffset, 6u);
  EXPECT_TRUE(breaks[0].requiresInsertedHyphen);
}
