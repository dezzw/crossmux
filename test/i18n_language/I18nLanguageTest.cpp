#include <gtest/gtest.h>

#include "I18n.h"

TEST(I18nLanguage, UnknownLanguageCodeFallsBackToEnglish) {
  EXPECT_EQ(I18n::languageFromCode("OOK"), Language::EN);
  EXPECT_EQ(I18n::languageFromCode("orangutan"), Language::EN);
}

TEST(I18nLanguage, EnglishCodeResolves) { EXPECT_EQ(I18n::languageFromCode("EN"), Language::EN); }
