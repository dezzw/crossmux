#include <gtest/gtest.h>

#include <algorithm>
#include <string>

#include "lib/Epub/Epub/hyphenation/Hyphenator.h"
#include "lib/Epub/Epub/hyphenation/LanguageRegistry.h"

namespace {

void setUncompiledPreferredLanguage() {
  Hyphenator::setPreferredLanguage("de");
  ASSERT_EQ(getLanguageHyphenatorForPrimaryTag("de"), nullptr);
}

bool hasBreakAtOrAfter(const std::vector<Hyphenator::BreakInfo>& breaks, const size_t byteOffset) {
  return std::any_of(breaks.begin(), breaks.end(),
                     [byteOffset](const Hyphenator::BreakInfo& info) { return info.byteOffset >= byteOffset; });
}

bool hasFallbackStyleBreak(const std::vector<Hyphenator::BreakInfo>& breaks) {
  return std::any_of(breaks.begin(), breaks.end(),
                     [](const Hyphenator::BreakInfo& info) { return !info.requiresInsertedHyphen; });
}

}  // namespace

TEST(NullHyphenatorFallback, GenericFallbackWhenIncludeFallbackEnabled) {
  setUncompiledPreferredLanguage();
  const std::string word = "Quadratkilometer";
  const auto breaks = Hyphenator::breakOffsets(word, true);

  ASSERT_FALSE(breaks.empty());
  EXPECT_TRUE(hasFallbackStyleBreak(breaks));
  EXPECT_TRUE(std::none_of(breaks.begin(), breaks.end(),
                           [](const Hyphenator::BreakInfo& info) { return info.requiresInsertedHyphen; }));
}

TEST(NullHyphenatorFallback, ExplicitHyphenPathWithoutLanguageHyphenator) {
  setUncompiledPreferredLanguage();
  const std::string word = "US-abcdefghijklmnop";
  const auto legalBreaks = Hyphenator::breakOffsets(word, false);

  ASSERT_FALSE(legalBreaks.empty());
  EXPECT_TRUE(hasBreakAtOrAfter(legalBreaks, 3u));
  EXPECT_TRUE(std::none_of(legalBreaks.begin(), legalBreaks.end(),
                           [](const Hyphenator::BreakInfo& info) { return info.requiresInsertedHyphen; }));
}

TEST(NullHyphenatorFallback, ExplicitHyphenPathAddsGenericFallbackWhenRequested) {
  setUncompiledPreferredLanguage();
  const std::string word = "US-abcdefghijklmnop";
  const auto legalOnly = Hyphenator::breakOffsets(word, false);
  const auto withFallback = Hyphenator::breakOffsets(word, true);

  ASSERT_FALSE(withFallback.empty());
  ASSERT_GE(withFallback.size(), legalOnly.size());
  EXPECT_GT(withFallback.size(), legalOnly.size());
}

TEST(NullHyphenatorFallback, ApostrophePathWithoutLanguageHyphenator) {
  setUncompiledPreferredLanguage();
  const std::string word = "quel'abcdefghijklmnop";
  const auto legalBreaks = Hyphenator::breakOffsets(word, false);

  ASSERT_FALSE(legalBreaks.empty());
  EXPECT_TRUE(hasBreakAtOrAfter(legalBreaks, 5u));
}

TEST(NullHyphenatorFallback, ApostrophePathAddsGenericFallbackWhenRequested) {
  setUncompiledPreferredLanguage();
  const std::string word = "quel'abcdefghijklmnop";
  const auto legalOnly = Hyphenator::breakOffsets(word, false);
  const auto withFallback = Hyphenator::breakOffsets(word, true);

  ASSERT_FALSE(withFallback.empty());
  EXPECT_GT(withFallback.size(), legalOnly.size());
}
