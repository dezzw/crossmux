#include <ArduinoJson.h>
#include <gtest/gtest.h>

#include "network/FontManifestCatalog.h"

namespace {

constexpr const char* kMinimalManifestTemplate = R"json({
  "version": 1,
  "baseUrl": "%s",
  "scriptGroups": [{"tag": "latn", "label": "Latin"}],
  "families": [{
    "name": "%s",
    "description": "Test family",
    "scripts": ["latn"],
    "files": [{"name": "%s_12.cpfont", "size": 1000, "crc32": 305419896}]
  }]
})json";

bool loadManifest(const char* baseUrl, const char* familyName, JsonDocument& doc) {
  char buffer[512];
  snprintf(buffer, sizeof(buffer), kMinimalManifestTemplate, baseUrl, familyName, familyName);
  return deserializeJson(doc, buffer) == DeserializationError::Ok;
}

}  // namespace

TEST(FontManifestCatalog, MergeAddsMissingFamilyFromSecondaryManifest) {
  JsonDocument githubDoc;
  JsonDocument legacyDoc;
  ASSERT_TRUE(loadManifest("https://github.com/example/sd-fonts-m1-b4/", "Literata", githubDoc));
  ASSERT_TRUE(loadManifest("https://assets.crossmux.cn/fonts/m1-b4/releases/test/", "NotoSansSC", legacyDoc));

  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(githubDoc, catalog, FontManifestCatalog::IngestMode::Replace),
            FontManifestCatalog::IngestResult::Ok);
  ASSERT_EQ(
      FontManifestCatalog::ingestDocument(legacyDoc, catalog, FontManifestCatalog::IngestMode::MergeSkipDuplicateNames),
      FontManifestCatalog::IngestResult::Ok);

  bool hasLiterata = false;
  bool hasNotoSansSc = false;
  std::string literataBase;
  std::string notoBase;
  for (const auto& family : catalog.families) {
    if (family.name == "Literata") {
      hasLiterata = true;
      literataBase = family.baseUrl;
    }
    if (family.name == "NotoSansSC") {
      hasNotoSansSc = true;
      notoBase = family.baseUrl;
    }
  }
  EXPECT_TRUE(hasLiterata);
  EXPECT_TRUE(hasNotoSansSc);
  EXPECT_NE(literataBase, notoBase);
}

TEST(FontManifestCatalog, MergePrefersPrimaryOnDuplicateName) {
  JsonDocument githubDoc;
  JsonDocument legacyDoc;
  ASSERT_TRUE(loadManifest("https://github.com/example/primary/", "Literata", githubDoc));
  ASSERT_TRUE(loadManifest("https://assets.crossmux.cn/legacy/", "Literata", legacyDoc));

  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(githubDoc, catalog, FontManifestCatalog::IngestMode::Replace),
            FontManifestCatalog::IngestResult::Ok);
  ASSERT_EQ(
      FontManifestCatalog::ingestDocument(legacyDoc, catalog, FontManifestCatalog::IngestMode::MergeSkipDuplicateNames),
      FontManifestCatalog::IngestResult::Ok);

  ASSERT_EQ(catalog.families.size(), 1u);
  EXPECT_EQ(catalog.families.front().baseUrl, "https://github.com/example/primary/");
}

TEST(FontManifestCatalog, ProductionMergeFitsCatalogLimits) {
  // GitHub m1-b4 ships 34 Latin families (146 files); crossmux.com legacy adds 4 CJK families (24 files).
  EXPECT_GE(FontManifestCatalog::Limits::kMaxManifestFamilies, 38u);
  EXPECT_GE(FontManifestCatalog::Limits::kMaxManifestFiles, 170u);
}

TEST(FontManifestCatalog, ParsePointSize) {
  uint8_t pointSize = 0;
  EXPECT_TRUE(FontManifestCatalog::parsePointSize("NotoSansSC", "NotoSansSC_16.cpfont", pointSize));
  EXPECT_EQ(pointSize, 16);
}
