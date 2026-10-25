#include <CrossPointSettings.h>

#include <ArduinoJson.h>
#include <gtest/gtest.h>

#include <I18nKeys.h>

extern int g_crosspoint_test_write_count;
extern void crosspoint_test_set_settings_json(const char* json);

namespace {

// Fields that skip legacy fromJson migrations unrelated to language (see CrossPointSettings.cpp).
constexpr char kStablePrefix[] =
    R"({"fakeBoldVersion":1,"statusBarChapterPageCount":1,"fontSize":12,"fontFamily":1)";

std::string docWithLanguage(const char* languageCode, int contentProfile) {
  std::string json = kStablePrefix;
  json += R"(,"contentProfile":)";
  json += std::to_string(contentProfile);
  json += R"(,"language":")";
  json += languageCode;
  json += R"("})";
  return json;
}

void loadLanguageDoc(const char* languageCode, int contentProfile) {
  const std::string json = docWithLanguage(languageCode, contentProfile);
  crosspoint_test_set_settings_json(json.c_str());
  g_crosspoint_test_write_count = 0;
  ASSERT_TRUE(CrossPointSettings::getInstance().loadFromFile());
}

}  // namespace

TEST(CrossPointSettingsLanguageJsonTest, StoredFrenchCodeRewritesToEnglish) {
  loadLanguageDoc("FR", 0);
  EXPECT_EQ(CrossPointSettings::getInstance().language, static_cast<uint8_t>(Language::EN));
  EXPECT_EQ(g_crosspoint_test_write_count, 1);
}

TEST(CrossPointSettingsLanguageJsonTest, StoredEnglishCodeNeedsNoResave) {
  loadLanguageDoc("EN", 0);
  EXPECT_EQ(CrossPointSettings::getInstance().language, static_cast<uint8_t>(Language::EN));
  EXPECT_EQ(g_crosspoint_test_write_count, 0);
}

TEST(CrossPointSettingsLanguageJsonTest, StoredChineseCodeNeedsNoResave) {
  loadLanguageDoc("ZH_CN", 1);
  EXPECT_EQ(CrossPointSettings::getInstance().language, static_cast<uint8_t>(Language::ZH_CN));
  EXPECT_EQ(g_crosspoint_test_write_count, 0);
}

TEST(CrossPointSettingsLanguageJsonTest, LowercaseEnglishCodeTriggersResave) {
  loadLanguageDoc("en", 0);
  EXPECT_EQ(CrossPointSettings::getInstance().language, static_cast<uint8_t>(Language::EN));
  EXPECT_EQ(g_crosspoint_test_write_count, 1);
}

TEST(CrossPointSettingsLanguageJsonTest, UnknownLanguageCodeFallsBackToEnglishAndResaves) {
  loadLanguageDoc("XX", 0);
  EXPECT_EQ(CrossPointSettings::getInstance().language, static_cast<uint8_t>(Language::EN));
  EXPECT_EQ(g_crosspoint_test_write_count, 1);
}
