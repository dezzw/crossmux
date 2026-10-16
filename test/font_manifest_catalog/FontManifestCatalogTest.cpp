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

constexpr const char* kLegacyManifestTemplate = R"json({
  "version": 1,
  "baseUrl": "%s",
  "families": [{
    "name": "%s",
    "description": "CJK family",
    "files": [{"name": "%s_12.cpfont", "size": 1000, "crc32": 305419896}]
  }]
})json";

bool loadManifest(const char* baseUrl, const char* familyName, JsonDocument& doc) {
  char buffer[512];
  snprintf(buffer, sizeof(buffer), kMinimalManifestTemplate, baseUrl, familyName, familyName);
  return deserializeJson(doc, buffer) == DeserializationError::Ok;
}

bool loadLegacyManifest(const char* baseUrl, const char* familyName, JsonDocument& doc) {
  char buffer[512];
  snprintf(buffer, sizeof(buffer), kLegacyManifestTemplate, baseUrl, familyName, familyName);
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

TEST(FontManifestCatalog, MergeAddsMissingFamilyFromSecondaryManifest) {
  JsonDocument legacyDoc;
  JsonDocument githubDoc;
  ASSERT_TRUE(loadLegacyManifest("https://assets.crossmux.cn/fonts/m1-b4/releases/test/", "NotoSansSC", legacyDoc));
  ASSERT_TRUE(loadManifest("https://github.com/example/sd-fonts-m1-b4/", "Literata", githubDoc));

  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(legacyDoc, catalog, FontManifestCatalog::IngestMode::Replace, "legacy"),
            FontManifestCatalog::IngestResult::Ok);
  ASSERT_EQ(FontManifestCatalog::ingestDocument(githubDoc, catalog,
                                                FontManifestCatalog::IngestMode::MergeReplaceDuplicateNames, "github"),
            FontManifestCatalog::IngestResult::Ok);

  bool hasLiterata = false;
  bool hasNotoSansSc = false;
  for (const auto& family : catalog.families) {
    if (family.name == "Literata") hasLiterata = true;
    if (family.name == "NotoSansSC") hasNotoSansSc = true;
  }
  EXPECT_TRUE(hasLiterata);
  EXPECT_TRUE(hasNotoSansSc);
  EXPECT_EQ(catalog.baseUrls.size(), 2u);
}

TEST(FontManifestCatalog, MergePrefersGithubOnDuplicateName) {
  JsonDocument legacyDoc;
  JsonDocument githubDoc;
  ASSERT_TRUE(loadLegacyManifest("https://assets.crossmux.cn/legacy/", "Literata", legacyDoc));
  ASSERT_TRUE(loadManifest("https://github.com/example/primary/", "Literata", githubDoc));

  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(legacyDoc, catalog, FontManifestCatalog::IngestMode::Replace, "legacy"),
            FontManifestCatalog::IngestResult::Ok);
  const uint8_t legacyBaseIndex = catalog.families.front().baseUrlIndex;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(githubDoc, catalog,
                                                FontManifestCatalog::IngestMode::MergeReplaceDuplicateNames, "github"),
            FontManifestCatalog::IngestResult::Ok);

  ASSERT_EQ(catalog.families.size(), 1u);
  EXPECT_NE(catalog.families.front().baseUrlIndex, legacyBaseIndex);
  EXPECT_STREQ(FontManifestCatalog::baseUrlForFamily(catalog, catalog.families.front()),
               "https://github.com/example/primary/");
}

TEST(FontManifestCatalog, ProductionMergeFitsCatalogLimits) {
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
  ASSERT_EQ(FontManifestCatalog::ingestDocument(doc, catalog, FontManifestCatalog::IngestMode::Replace, "overflow"),
            FontManifestCatalog::IngestResult::Overflow);
  EXPECT_TRUE(catalog.families.empty());
  EXPECT_TRUE(catalog.files.empty());
}

TEST(FontManifestCatalog, MidIngestFailureLeavesPriorCatalogIntact) {
  JsonDocument legacyDoc;
  ASSERT_TRUE(loadLegacyManifest("https://assets.crossmux.cn/legacy/", "NotoSansSC", legacyDoc));

  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(legacyDoc, catalog, FontManifestCatalog::IngestMode::Replace, "legacy"),
            FontManifestCatalog::IngestResult::Ok);
  ASSERT_EQ(catalog.families.size(), 1u);

  JsonDocument badGithub;
  badGithub["version"] = 1;
  badGithub["baseUrl"] = "https://github.com/example/";
  JsonArray groups = badGithub["scriptGroups"].to<JsonArray>();
  JsonObject group = groups.add<JsonObject>();
  group["tag"] = "latn";
  group["label"] = "Latin";
  JsonArray families = badGithub["families"].to<JsonArray>();
  JsonObject good = families.add<JsonObject>();
  good["name"] = "GoodFamily";
  good["description"] = "ok";
  JsonArray goodScripts = good["scripts"].to<JsonArray>();
  goodScripts.add("latn");
  JsonArray goodFiles = good["files"].to<JsonArray>();
  JsonObject goodFile = goodFiles.add<JsonObject>();
  goodFile["name"] = "GoodFamily_12.cpfont";
  goodFile["size"] = 1000;
  goodFile["crc32"] = 305419896;
  JsonObject bad = families.add<JsonObject>();
  bad["name"] = "BadFamily";
  bad["description"] = "bad";
  JsonArray badFiles = bad["files"].to<JsonArray>();
  JsonObject badFile = badFiles.add<JsonObject>();
  badFile["name"] = "BadFamily_12.cpfont";
  badFile["size"] = 0;
  badFile["crc32"] = 305419896;

  ASSERT_EQ(FontManifestCatalog::ingestDocument(badGithub, catalog,
                                                FontManifestCatalog::IngestMode::MergeReplaceDuplicateNames, "github"),
            FontManifestCatalog::IngestResult::Parse);
  ASSERT_EQ(catalog.families.size(), 1u);
  EXPECT_EQ(catalog.families.front().name, "NotoSansSC");
}

TEST(FontManifestCatalog, LegacyOnlyThenGithubFailureKeepsLegacy) {
  JsonDocument legacyDoc;
  ASSERT_TRUE(loadLegacyManifest("https://assets.crossmux.cn/legacy/", "NotoSansSC", legacyDoc));
  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(legacyDoc, catalog, FontManifestCatalog::IngestMode::Replace, "legacy"),
            FontManifestCatalog::IngestResult::Ok);

  JsonDocument badVersion;
  badVersion["version"] = 99;
  badVersion["baseUrl"] = "https://github.com/example/";
  badVersion["families"].to<JsonArray>();
  ASSERT_EQ(FontManifestCatalog::ingestDocument(badVersion, catalog,
                                                FontManifestCatalog::IngestMode::MergeReplaceDuplicateNames, "github"),
            FontManifestCatalog::IngestResult::Version);
  ASSERT_EQ(catalog.families.size(), 1u);
}

TEST(FontManifestCatalog, GithubOnlyWhenLegacyFails) {
  JsonDocument githubDoc;
  ASSERT_TRUE(loadManifest("https://github.com/example/primary/", "Literata", githubDoc));

  JsonDocument badLegacy;
  badLegacy["version"] = 1;
  badLegacy["baseUrl"] = "https://assets.crossmux.cn/legacy/";
  JsonArray families = badLegacy["families"].to<JsonArray>();
  JsonObject bad = families.add<JsonObject>();
  bad["name"] = "Bad";
  bad["description"] = "bad";
  JsonArray badFiles = bad["files"].to<JsonArray>();
  JsonObject badFile = badFiles.add<JsonObject>();
  badFile["name"] = "Bad_12.cpfont";
  badFile["size"] = 0;
  badFile["crc32"] = 1;

  FontManifestCatalog::Catalog catalog;
  ASSERT_NE(FontManifestCatalog::ingestDocument(badLegacy, catalog, FontManifestCatalog::IngestMode::Replace, "legacy"),
            FontManifestCatalog::IngestResult::Ok);
  ASSERT_EQ(FontManifestCatalog::ingestDocument(githubDoc, catalog,
                                                FontManifestCatalog::IngestMode::MergeReplaceDuplicateNames, "github"),
            FontManifestCatalog::IngestResult::Ok);
  ASSERT_EQ(catalog.families.size(), 1u);
  EXPECT_EQ(catalog.families.front().name, "Literata");
}

TEST(FontManifestCatalog, UnscriptedFamiliesDefaultToCjkGroup) {
  JsonDocument legacyDoc;
  ASSERT_TRUE(loadLegacyManifest("https://assets.crossmux.cn/legacy/", "NotoSansSC", legacyDoc));
  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(legacyDoc, catalog, FontManifestCatalog::IngestMode::Replace, "legacy"),
            FontManifestCatalog::IngestResult::Ok);
  EXPECT_EQ(catalog.families.front().scriptMask, 0u);
  FontManifestCatalog::applyDefaultScriptGroupForUnscriptedFamilies(catalog);
  ASSERT_FALSE(catalog.scriptGroupTags.empty());
  EXPECT_EQ(catalog.scriptGroupTags.front(), "cjk");
  EXPECT_NE(catalog.families.front().scriptMask, 0u);
}

TEST(FontManifestCatalog, SharesBaseUrlAcrossFamiliesFromSameSource) {
  JsonDocument doc;
  doc["version"] = 1;
  doc["baseUrl"] = "https://example.com/shared/";
  JsonArray groups = doc["scriptGroups"].to<JsonArray>();
  JsonObject group = groups.add<JsonObject>();
  group["tag"] = "latn";
  group["label"] = "Latin";
  JsonArray families = doc["families"].to<JsonArray>();
  for (const char* name : {"Alpha", "Beta"}) {
    JsonObject family = families.add<JsonObject>();
    family["name"] = name;
    family["description"] = "desc";
    JsonArray scripts = family["scripts"].to<JsonArray>();
    scripts.add("latn");
    JsonArray files = family["files"].to<JsonArray>();
    JsonObject file = files.add<JsonObject>();
    char fileName[32];
    snprintf(fileName, sizeof(fileName), "%s_12.cpfont", name);
    file["name"] = fileName;
    file["size"] = 1000;
    file["crc32"] = 305419896;
  }

  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(doc, catalog, FontManifestCatalog::IngestMode::Replace, "shared"),
            FontManifestCatalog::IngestResult::Ok);
  ASSERT_EQ(catalog.families.size(), 2u);
  EXPECT_EQ(catalog.baseUrls.size(), 1u);
  EXPECT_EQ(catalog.families[0].baseUrlIndex, catalog.families[1].baseUrlIndex);
}
