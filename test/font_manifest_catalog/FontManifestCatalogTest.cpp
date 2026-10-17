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

bool loadManyFamiliesManifest(size_t familyCount, JsonDocument& doc) {
  JsonDocument builder;
  builder["version"] = 1;
  builder["baseUrl"] = "https://example.com/fonts/";
  JsonArray groups = builder["scriptGroups"].to<JsonArray>();
  JsonObject group = groups.add<JsonObject>();
  group["tag"] = "latn";
  group["label"] = "Latin";
  JsonArray families = builder["families"].to<JsonArray>();
  for (size_t index = 0; index < familyCount; index++) {
    JsonObject family = families.add<JsonObject>();
    char name[32];
    snprintf(name, sizeof(name), "Family%zu", index);
    family["name"] = name;
    family["description"] = "desc";
    JsonArray scripts = family["scripts"].to<JsonArray>();
    scripts.add("latn");
    JsonArray files = family["files"].to<JsonArray>();
    JsonObject file = files.add<JsonObject>();
    char fileName[48];
    snprintf(fileName, sizeof(fileName), "%s_12.cpfont", name);
    file["name"] = fileName;
    file["size"] = 1000;
    file["crc32"] = 305419896;
  }
  doc.set(builder);
  return true;
}

}  // namespace

TEST(FontManifestCatalog, ProductionCatalogFitsLimits) {
  EXPECT_GE(FontManifestCatalog::Limits::kMaxManifestFamilies, 64u);
  EXPECT_GE(FontManifestCatalog::Limits::kMaxManifestFiles, 256u);
}

TEST(FontManifestCatalog, ParsePointSize) {
  uint8_t pointSize = 0;
  EXPECT_TRUE(FontManifestCatalog::parsePointSize("NotoSansSC", "NotoSansSC_16.cpfont", pointSize));
  EXPECT_EQ(pointSize, 16);
}

TEST(FontManifestCatalog, OverflowRejectsBeforeMutatingCatalog) {
  JsonDocument doc;
  ASSERT_TRUE(loadManyFamiliesManifest(FontManifestCatalog::Limits::kMaxManifestFamilies + 1, doc));
  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(doc, catalog, "overflow"), FontManifestCatalog::IngestResult::Overflow);
  EXPECT_TRUE(catalog.families.empty());
}

TEST(FontManifestCatalog, UnscriptedFamiliesDefaultToCjkGroup) {
  JsonDocument doc;
  doc["version"] = 1;
  doc["baseUrl"] = "https://github.com/dezzw/crossmux/releases/download/fonts-m1-b4/";
  JsonArray families = doc["families"].to<JsonArray>();
  JsonObject family = families.add<JsonObject>();
  family["name"] = "NotoSansSC";
  family["description"] = "CJK family";
  JsonArray files = family["files"].to<JsonArray>();
  JsonObject file = files.add<JsonObject>();
  file["name"] = "NotoSansSC_12.cpfont";
  file["size"] = 1000;
  file["crc32"] = 305419896;

  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(doc, catalog, "manifest"), FontManifestCatalog::IngestResult::Ok);
  FontManifestCatalog::applyDefaultScriptGroupForUnscriptedFamilies(catalog);
  ASSERT_EQ(catalog.scriptGroupTags.size(), 1u);
  EXPECT_EQ(catalog.scriptGroupTags.front(), FontManifestCatalog::kCjkScriptGroupTag);
  EXPECT_EQ(catalog.families.front().scriptMask, 1u);
}

TEST(FontManifestCatalog, ReplaceRejectsDuplicateFamilyNameInOneManifest) {
  JsonDocument doc;
  doc["version"] = 1;
  doc["baseUrl"] = "https://example.com/fonts/";
  JsonArray families = doc["families"].to<JsonArray>();
  for (int copy = 0; copy < 2; copy++) {
    JsonObject family = families.add<JsonObject>();
    family["name"] = "Literata";
    family["description"] = "dup";
    JsonArray files = family["files"].to<JsonArray>();
    JsonObject file = files.add<JsonObject>();
    file["name"] = "Literata_12.cpfont";
    file["size"] = 1000;
    file["crc32"] = 305419896;
  }
  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(doc, catalog, "dup"), FontManifestCatalog::IngestResult::Parse);
}

TEST(FontManifestCatalog, SharesBaseUrlAcrossFamiliesFromSameSource) {
  JsonDocument doc;
  doc["version"] = 1;
  doc["baseUrl"] = "https://github.com/dezzw/crossmux/releases/download/fonts-m1-b4/";
  JsonArray groups = doc["scriptGroups"].to<JsonArray>();
  JsonObject group = groups.add<JsonObject>();
  group["tag"] = "latn";
  group["label"] = "Latin";
  JsonArray families = doc["families"].to<JsonArray>();
  for (const char* name : {"Literata", "Inter"}) {
    JsonObject family = families.add<JsonObject>();
    family["name"] = name;
    family["description"] = "desc";
    JsonArray scripts = family["scripts"].to<JsonArray>();
    scripts.add("latn");
    JsonArray files = family["files"].to<JsonArray>();
    JsonObject file = files.add<JsonObject>();
    char fileName[48];
    snprintf(fileName, sizeof(fileName), "%s_12.cpfont", name);
    file["name"] = fileName;
    file["size"] = 1000;
    file["crc32"] = 305419896;
  }

  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(doc, catalog, "shared"), FontManifestCatalog::IngestResult::Ok);
  EXPECT_EQ(catalog.baseUrls.size(), 1u);
  EXPECT_STREQ(FontManifestCatalog::baseUrlForFamily(catalog, catalog.families.front()),
               "https://github.com/dezzw/crossmux/releases/download/fonts-m1-b4/");
}

TEST(FontManifestCatalog, IngestSingleFamilyManifest) {
  JsonDocument doc;
  ASSERT_TRUE(loadManifest("https://github.com/dezzw/crossmux/releases/download/fonts-m1-b4/", "Literata", doc));
  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(doc, catalog, "manifest"), FontManifestCatalog::IngestResult::Ok);
  ASSERT_EQ(catalog.families.size(), 1u);
  EXPECT_EQ(catalog.families.front().name, "Literata");
}
