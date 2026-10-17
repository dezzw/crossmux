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
  ASSERT_EQ(FontManifestCatalog::ingestDocument(legacyDoc, catalog, FontManifestCatalog::IngestMode::Replace, "legacy",
                                                FontManifestCatalog::ManifestBundleSource::CrossMux),
            FontManifestCatalog::IngestResult::Ok);
  ASSERT_EQ(FontManifestCatalog::ingestDocument(githubDoc, catalog,
                                                FontManifestCatalog::IngestMode::MergeReplaceDuplicateNames, "github",
                                                FontManifestCatalog::ManifestBundleSource::UpstreamGithub),
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
  ASSERT_EQ(FontManifestCatalog::ingestDocument(legacyDoc, catalog, FontManifestCatalog::IngestMode::Replace, "legacy",
                                                FontManifestCatalog::ManifestBundleSource::CrossMux),
            FontManifestCatalog::IngestResult::Ok);
  const uint8_t legacyBaseIndex = catalog.families.front().baseUrlIndex;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(githubDoc, catalog,
                                                FontManifestCatalog::IngestMode::MergeReplaceDuplicateNames, "github",
                                                FontManifestCatalog::ManifestBundleSource::UpstreamGithub),
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
  ASSERT_EQ(FontManifestCatalog::ingestDocument(doc, catalog, FontManifestCatalog::IngestMode::Replace, "overflow",
                                                FontManifestCatalog::ManifestBundleSource::UpstreamGithub),
            FontManifestCatalog::IngestResult::Overflow);
  EXPECT_TRUE(catalog.families.empty());
  EXPECT_TRUE(catalog.files.empty());
}

TEST(FontManifestCatalog, MidIngestFailureLeavesPriorCatalogIntact) {
  JsonDocument legacyDoc;
  ASSERT_TRUE(loadLegacyManifest("https://assets.crossmux.cn/legacy/", "NotoSansSC", legacyDoc));

  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(legacyDoc, catalog, FontManifestCatalog::IngestMode::Replace, "legacy",
                                                FontManifestCatalog::ManifestBundleSource::CrossMux),
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
                                                FontManifestCatalog::IngestMode::MergeReplaceDuplicateNames, "github",
                                                FontManifestCatalog::ManifestBundleSource::UpstreamGithub),
            FontManifestCatalog::IngestResult::Parse);
  ASSERT_EQ(catalog.families.size(), 1u);
  EXPECT_EQ(catalog.families.front().name, "NotoSansSC");
}

TEST(FontManifestCatalog, LegacyOnlyThenGithubFailureKeepsLegacy) {
  JsonDocument legacyDoc;
  ASSERT_TRUE(loadLegacyManifest("https://assets.crossmux.cn/legacy/", "NotoSansSC", legacyDoc));
  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(legacyDoc, catalog, FontManifestCatalog::IngestMode::Replace, "legacy",
                                                FontManifestCatalog::ManifestBundleSource::CrossMux),
            FontManifestCatalog::IngestResult::Ok);

  JsonDocument badVersion;
  badVersion["version"] = 99;
  badVersion["baseUrl"] = "https://github.com/example/";
  badVersion["families"].to<JsonArray>();
  ASSERT_EQ(FontManifestCatalog::ingestDocument(badVersion, catalog,
                                                FontManifestCatalog::IngestMode::MergeReplaceDuplicateNames, "github",
                                                FontManifestCatalog::ManifestBundleSource::UpstreamGithub),
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
  ASSERT_NE(FontManifestCatalog::ingestDocument(badLegacy, catalog, FontManifestCatalog::IngestMode::Replace, "legacy",
                                                FontManifestCatalog::ManifestBundleSource::CrossMux),
            FontManifestCatalog::IngestResult::Ok);
  ASSERT_EQ(FontManifestCatalog::ingestDocument(githubDoc, catalog,
                                                FontManifestCatalog::IngestMode::MergeReplaceDuplicateNames, "github",
                                                FontManifestCatalog::ManifestBundleSource::UpstreamGithub),
            FontManifestCatalog::IngestResult::Ok);
  ASSERT_EQ(catalog.families.size(), 1u);
  EXPECT_EQ(catalog.families.front().name, "Literata");
}

TEST(FontManifestCatalog, UnscriptedFamiliesDefaultToCjkGroup) {
  JsonDocument legacyDoc;
  ASSERT_TRUE(loadLegacyManifest("https://assets.crossmux.cn/legacy/", "NotoSansSC", legacyDoc));
  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(legacyDoc, catalog, FontManifestCatalog::IngestMode::Replace, "legacy",
                                                FontManifestCatalog::ManifestBundleSource::CrossMux),
            FontManifestCatalog::IngestResult::Ok);
  EXPECT_EQ(catalog.families.front().scriptMask, 0u);
  FontManifestCatalog::applyDefaultScriptGroupForUnscriptedFamilies(catalog);
  ASSERT_FALSE(catalog.scriptGroupTags.empty());
  EXPECT_EQ(catalog.scriptGroupTags.back(), FontManifestCatalog::kCjkScriptGroupTag);
  EXPECT_NE(catalog.families.front().scriptMask, 0u);
}

TEST(FontManifestCatalog, UnscriptedCrossMuxBundleOnForeignHostGetsCjk) {
  JsonDocument legacyDoc;
  legacyDoc["version"] = 1;
  legacyDoc["baseUrl"] = "https://unrelated.cdn.example/fonts/";
  JsonArray families = legacyDoc["families"].to<JsonArray>();
  JsonObject family = families.add<JsonObject>();
  family["name"] = "NotoSansSC";
  family["description"] = "CJK family";
  JsonArray files = family["files"].to<JsonArray>();
  JsonObject file = files.add<JsonObject>();
  file["name"] = "NotoSansSC_12.cpfont";
  file["size"] = 1000;
  file["crc32"] = 305419896;

  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(legacyDoc, catalog, FontManifestCatalog::IngestMode::Replace, "legacy",
                                                FontManifestCatalog::ManifestBundleSource::CrossMux),
            FontManifestCatalog::IngestResult::Ok);
  FontManifestCatalog::applyDefaultScriptGroupForUnscriptedFamilies(catalog);
  EXPECT_NE(catalog.families.front().scriptMask, 0u);
}

TEST(FontManifestCatalog, UnscriptedGithubFamilyStaysInAllGroup) {
  JsonDocument githubDoc;
  githubDoc["version"] = 1;
  githubDoc["baseUrl"] = "https://github.com/example/sd-fonts-m1-b4/";
  JsonArray families = githubDoc["families"].to<JsonArray>();
  JsonObject family = families.add<JsonObject>();
  family["name"] = "UnscriptedGithub";
  family["description"] = "No scripts field";
  JsonArray files = family["files"].to<JsonArray>();
  JsonObject file = files.add<JsonObject>();
  file["name"] = "UnscriptedGithub_12.cpfont";
  file["size"] = 1000;
  file["crc32"] = 305419896;

  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(githubDoc, catalog, FontManifestCatalog::IngestMode::Replace, "github",
                                                FontManifestCatalog::ManifestBundleSource::UpstreamGithub),
            FontManifestCatalog::IngestResult::Ok);
  FontManifestCatalog::applyDefaultScriptGroupForUnscriptedFamilies(catalog);
  EXPECT_EQ(catalog.families.front().scriptMask, 0u);
  EXPECT_TRUE(catalog.scriptGroupTags.empty());
}

TEST(FontManifestCatalog, MergeReplaceDuplicateInMiddleShiftsFileOffsets) {
  JsonDocument legacyDoc;
  legacyDoc["version"] = 1;
  legacyDoc["baseUrl"] = "https://assets.crossmux.cn/legacy/";
  JsonArray legacyFamilies = legacyDoc["families"].to<JsonArray>();
  for (const char* name : {"Alpha", "DupMiddle", "Omega"}) {
    JsonObject family = legacyFamilies.add<JsonObject>();
    family["name"] = name;
    family["description"] = "legacy";
    JsonArray files = family["files"].to<JsonArray>();
    JsonObject file = files.add<JsonObject>();
    char fileName[32];
    snprintf(fileName, sizeof(fileName), "%s_12.cpfont", name);
    file["name"] = fileName;
    file["size"] = 1000;
    file["crc32"] = 305419896;
  }

  JsonDocument githubDoc;
  githubDoc["version"] = 1;
  githubDoc["baseUrl"] = "https://github.com/example/replace/";
  JsonArray githubFamilies = githubDoc["families"].to<JsonArray>();
  JsonObject replaced = githubFamilies.add<JsonObject>();
  replaced["name"] = "DupMiddle";
  replaced["description"] = "github";
  JsonArray scripts = replaced["scripts"].to<JsonArray>();
  scripts.add("latn");
  JsonArray replacedFiles = replaced["files"].to<JsonArray>();
  for (unsigned pointSize : {12u, 14u}) {
    JsonObject file = replacedFiles.add<JsonObject>();
    char fileName[32];
    snprintf(fileName, sizeof(fileName), "DupMiddle_%u.cpfont", pointSize);
    file["name"] = fileName;
    file["size"] = 1000 + pointSize;
    file["crc32"] = 305419896;
  }
  JsonArray groups = githubDoc["scriptGroups"].to<JsonArray>();
  JsonObject group = groups.add<JsonObject>();
  group["tag"] = "latn";
  group["label"] = "Latin";

  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(legacyDoc, catalog, FontManifestCatalog::IngestMode::Replace, "legacy",
                                                FontManifestCatalog::ManifestBundleSource::CrossMux),
            FontManifestCatalog::IngestResult::Ok);
  ASSERT_EQ(catalog.families.size(), 3u);
  ASSERT_EQ(catalog.files.size(), 3u);
  ASSERT_EQ(FontManifestCatalog::ingestDocument(githubDoc, catalog,
                                                FontManifestCatalog::IngestMode::MergeReplaceDuplicateNames, "github",
                                                FontManifestCatalog::ManifestBundleSource::UpstreamGithub),
            FontManifestCatalog::IngestResult::Ok);

  ASSERT_EQ(catalog.families.size(), 3u);
  ASSERT_EQ(catalog.files.size(), 4u);
  EXPECT_EQ(catalog.families[0].name, "Alpha");
  EXPECT_EQ(catalog.families[0].fileOffset, 0u);
  EXPECT_EQ(catalog.families[0].fileCount, 1u);
  EXPECT_EQ(catalog.families[1].name, "DupMiddle");
  EXPECT_EQ(catalog.families[1].fileOffset, 2u);
  EXPECT_EQ(catalog.families[1].fileCount, 2u);
  EXPECT_EQ(catalog.families[2].name, "Omega");
  EXPECT_EQ(catalog.families[2].fileOffset, 1u);
  EXPECT_EQ(catalog.families[2].fileCount, 1u);
  EXPECT_EQ(catalog.files[2].pointSize, 12);
}

TEST(FontManifestCatalog, MergeSkipsDuplicateScriptTagWithinManifest) {
  JsonDocument doc;
  doc["version"] = 1;
  doc["baseUrl"] = "https://github.com/example/fonts/";
  JsonArray groups = doc["scriptGroups"].to<JsonArray>();
  JsonObject first = groups.add<JsonObject>();
  first["tag"] = "latn";
  first["label"] = "Latin";
  JsonObject duplicate = groups.add<JsonObject>();
  duplicate["tag"] = "latn";
  duplicate["label"] = "Latin again";
  JsonArray families = doc["families"].to<JsonArray>();
  JsonObject family = families.add<JsonObject>();
  family["name"] = "Literata";
  family["description"] = "desc";
  JsonArray scripts = family["scripts"].to<JsonArray>();
  scripts.add("latn");
  JsonArray files = family["files"].to<JsonArray>();
  JsonObject file = files.add<JsonObject>();
  file["name"] = "Literata_12.cpfont";
  file["size"] = 1000;
  file["crc32"] = 305419896;

  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(
      FontManifestCatalog::ingestDocument(doc, catalog, FontManifestCatalog::IngestMode::MergeReplaceDuplicateNames,
                                          "github", FontManifestCatalog::ManifestBundleSource::UpstreamGithub),
      FontManifestCatalog::IngestResult::Ok);
  ASSERT_EQ(catalog.scriptGroupTags.size(), 1u);
  EXPECT_EQ(catalog.scriptGroupTags.front(), "latn");
  EXPECT_EQ(catalog.scriptGroupLabels.front(), "Latin");
}

TEST(FontManifestCatalog, MergeMalformedScriptGroupsLeavePriorCatalogUntouched) {
  JsonDocument legacyDoc;
  ASSERT_TRUE(loadLegacyManifest("https://assets.crossmux.cn/legacy/", "NotoSansSC", legacyDoc));
  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(legacyDoc, catalog, FontManifestCatalog::IngestMode::Replace, "legacy",
                                                FontManifestCatalog::ManifestBundleSource::CrossMux),
            FontManifestCatalog::IngestResult::Ok);
  const size_t tagsBefore = catalog.scriptGroupTags.size();

  JsonDocument githubDoc;
  githubDoc["version"] = 1;
  githubDoc["baseUrl"] = "https://github.com/example/replace/";
  JsonArray groups = githubDoc["scriptGroups"].to<JsonArray>();
  JsonObject good = groups.add<JsonObject>();
  good["tag"] = "latn";
  good["label"] = "Latin";
  JsonObject bad = groups.add<JsonObject>();
  bad["tag"] = "hangul";
  bad["label"] = "";
  JsonArray families = githubDoc["families"].to<JsonArray>();
  JsonObject family = families.add<JsonObject>();
  family["name"] = "Literata";
  family["description"] = "desc";
  JsonArray scripts = family["scripts"].to<JsonArray>();
  scripts.add("latn");
  JsonArray files = family["files"].to<JsonArray>();
  JsonObject file = files.add<JsonObject>();
  file["name"] = "Literata_12.cpfont";
  file["size"] = 1000;
  file["crc32"] = 305419896;

  ASSERT_EQ(FontManifestCatalog::ingestDocument(githubDoc, catalog,
                                                FontManifestCatalog::IngestMode::MergeReplaceDuplicateNames, "github",
                                                FontManifestCatalog::ManifestBundleSource::UpstreamGithub),
            FontManifestCatalog::IngestResult::Parse);
  EXPECT_EQ(catalog.scriptGroupTags.size(), tagsBefore);
  ASSERT_EQ(catalog.families.size(), 1u);
  EXPECT_EQ(catalog.families.front().name, "NotoSansSC");
}

TEST(FontManifestCatalog, ReplaceRejectsDuplicateFamilyNameInOneManifest) {
  JsonDocument doc;
  doc["version"] = 1;
  doc["baseUrl"] = "https://example.com/fonts/";
  JsonArray families = doc["families"].to<JsonArray>();
  for (unsigned pointSize : {12u, 14u}) {
    JsonObject family = families.add<JsonObject>();
    family["name"] = "A";
    family["description"] = "desc";
    JsonArray files = family["files"].to<JsonArray>();
    JsonObject file = files.add<JsonObject>();
    char fileName[16];
    snprintf(fileName, sizeof(fileName), "A_%u.cpfont", pointSize);
    file["name"] = fileName;
    file["size"] = 1000;
    file["crc32"] = 305419896;
  }

  FontManifestCatalog::Catalog catalog;
  ASSERT_EQ(FontManifestCatalog::ingestDocument(doc, catalog, FontManifestCatalog::IngestMode::Replace, "dup",
                                                FontManifestCatalog::ManifestBundleSource::UpstreamGithub),
            FontManifestCatalog::IngestResult::Parse);
  EXPECT_TRUE(catalog.families.empty());
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
  ASSERT_EQ(FontManifestCatalog::ingestDocument(doc, catalog, FontManifestCatalog::IngestMode::Replace, "shared",
                                                FontManifestCatalog::ManifestBundleSource::UpstreamGithub),
            FontManifestCatalog::IngestResult::Ok);
  ASSERT_EQ(catalog.families.size(), 2u);
  EXPECT_EQ(catalog.baseUrls.size(), 1u);
  EXPECT_EQ(catalog.families[0].baseUrlIndex, catalog.families[1].baseUrlIndex);
}
