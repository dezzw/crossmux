#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "lib/Epub/Epub/hyphenation/Hyphenator.h"
#include "lib/Epub/Epub/hyphenation/LanguageRegistry.h"

namespace {

void setUncompiledPreferredLanguage() {
  Hyphenator::setPreferredLanguage("de");
  ASSERT_EQ(getLanguageHyphenatorForPrimaryTag("de"), nullptr);
}

using BreakPair = std::pair<size_t, bool>;

std::vector<BreakPair> asBreakPairs(const std::vector<Hyphenator::BreakInfo>& breaks) {
  std::vector<BreakPair> pairs;
  pairs.reserve(breaks.size());
  for (const auto& info : breaks) {
    pairs.emplace_back(info.byteOffset, info.requiresInsertedHyphen);
  }
  return pairs;
}

void expectExactBreaks(const std::vector<Hyphenator::BreakInfo>& breaks, const std::vector<BreakPair>& expected) {
  const auto actual = asBreakPairs(breaks);
  ASSERT_EQ(actual.size(), expected.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(actual[i].first, expected[i].first) << " at index " << i;
    EXPECT_EQ(actual[i].second, expected[i].second) << " at index " << i;
  }
}

}  // namespace

TEST(NullHyphenatorFallback, GenericFallbackWhenIncludeFallbackEnabled) {
  setUncompiledPreferredLanguage();
  const auto breaks = Hyphenator::breakOffsets("Quadratkilometer", true);

  expectExactBreaks(breaks, {{2, false},
                             {3, false},
                             {4, false},
                             {5, false},
                             {6, false},
                             {7, false},
                             {8, false},
                             {9, false},
                             {10, false},
                             {11, false},
                             {12, false},
                             {13, false},
                             {14, false}});
}

TEST(NullHyphenatorFallback, ExplicitHyphenPathWithoutLanguageHyphenator) {
  setUncompiledPreferredLanguage();
  const auto legalBreaks = Hyphenator::breakOffsets("US-abcdefghijklmnop", false);

  expectExactBreaks(legalBreaks, {{3, false}});
}

TEST(NullHyphenatorFallback, ExplicitHyphenPathAddsGenericFallbackWhenRequested) {
  setUncompiledPreferredLanguage();
  const auto withFallback = Hyphenator::breakOffsets("US-abcdefghijklmnop", true);

  expectExactBreaks(withFallback, {{2, false},
                                   {3, false},
                                   {4, false},
                                   {5, false},
                                   {6, false},
                                   {7, false},
                                   {8, false},
                                   {9, false},
                                   {10, false},
                                   {11, false},
                                   {12, false},
                                   {13, false},
                                   {14, false},
                                   {15, false},
                                   {16, false},
                                   {17, false}});
}

TEST(NullHyphenatorFallback, ApostrophePathWithoutLanguageHyphenator) {
  setUncompiledPreferredLanguage();
  const auto legalBreaks = Hyphenator::breakOffsets("quel'abcdefghijklmnop", false);

  expectExactBreaks(legalBreaks, {{5, false}});
}

TEST(NullHyphenatorFallback, ApostrophePathAddsGenericFallbackWhenRequested) {
  setUncompiledPreferredLanguage();
  const auto withFallback = Hyphenator::breakOffsets("quel'abcdefghijklmnop", true);

  expectExactBreaks(withFallback, {{2, false},
                                   {3, false},
                                   {4, false},
                                   {5, false},
                                   {6, false},
                                   {7, false},
                                   {8, false},
                                   {9, false},
                                   {10, false},
                                   {11, false},
                                   {12, false},
                                   {13, false},
                                   {14, false},
                                   {15, false},
                                   {16, false},
                                   {17, false},
                                   {18, false},
                                   {19, false}});
}
