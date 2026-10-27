#include <I18n.h>
#include <I18nKeys.h>
#include <gtest/gtest.h>

#include "util/StoredLanguageCode.h"

TEST(I18nBuildLanguages, ShipsEnglishAndChineseOnly) {
  EXPECT_EQ(getLanguageCount(), 2u);
  EXPECT_STREQ(LANGUAGE_CODES[static_cast<size_t>(Language::EN)], "EN");
  EXPECT_STREQ(LANGUAGE_CODES[static_cast<size_t>(Language::ZH_CN)], "ZH_CN");
}

TEST(I18nBuildLanguages, ExcludedLanguageCodesFallBackToEnglish) {
  // settings.json stores _language_code strings; removed languages must not stick.
  EXPECT_EQ(I18n::languageFromCode("FR"), Language::EN);
  EXPECT_EQ(I18n::languageFromCode("DE"), Language::EN);
  EXPECT_EQ(I18n::languageFromCode("AR"), Language::EN);
  EXPECT_TRUE(I18n::isLanguageAvailable(Language::EN));
  EXPECT_TRUE(I18n::isLanguageAvailable(Language::ZH_CN));
}

TEST(I18nBuildLanguages, ObsoleteStoredLanguageCodeTriggersResave) {
  const Language parsed = I18n::languageFromCode("FR");
  EXPECT_EQ(parsed, Language::EN);
  EXPECT_TRUE(storedLanguageCodeNeedsResave("FR", parsed));
  EXPECT_FALSE(storedLanguageCodeNeedsResave("EN", Language::EN));
  EXPECT_FALSE(storedLanguageCodeNeedsResave("ZH_CN", Language::ZH_CN));
}
