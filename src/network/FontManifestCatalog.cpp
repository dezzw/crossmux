#include "FontManifestCatalog.h"

#include <Logging.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <vector>

namespace FontManifestCatalog {

namespace {

constexpr int kExpectedManifestSchemaVersion = 1;

const Catalog kEmptyCatalog{};

bool isCrossMuxBundleSource(const Catalog& catalog, uint8_t baseUrlIndex) {
  return baseUrlIndex < catalog.baseUrlSources.size() &&
         catalog.baseUrlSources[baseUrlIndex] == ManifestBundleSource::CrossMux;
}

bool isValidFamilyName(const char* name) {
  if (name == nullptr || name[0] == '\0') return false;
  if (strstr(name, "..") != nullptr) return false;
  if (strchr(name, '/') != nullptr) return false;
  if (strchr(name, '\\') != nullptr) return false;
  for (const char* p = name; *p != '\0'; ++p) {
    const char c = *p;
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_') return false;
  }
  return true;
}

bool isValidCpfontFilename(const char* name) {
  if (name == nullptr || name[0] == '\0') return false;
  if (strstr(name, "..") != nullptr) return false;
  if (strchr(name, '/') != nullptr) return false;
  if (strchr(name, '\\') != nullptr) return false;
  static constexpr char kExt[] = ".cpfont";
  static constexpr size_t kExtLen = sizeof(kExt) - 1;
  const size_t nameLen = strlen(name);
  if (nameLen <= kExtLen) return false;
  if (strcmp(name + nameLen - kExtLen, kExt) != 0) return false;
  const size_t baseLen = nameLen - kExtLen;
  for (size_t i = 0; i < baseLen; ++i) {
    const char c = name[i];
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_') return false;
  }
  return true;
}

size_t findScriptGroupIndex(const std::vector<std::string>& tags, const char* tag) {
  for (size_t index = 0; index < tags.size(); index++) {
    if (tags[index] == tag) return index;
  }
  return tags.size();
}

uint32_t scriptMaskForTags(const std::vector<std::string>& groupTags, JsonArrayConst scripts) {
  uint32_t mask = 0;
  if (scripts.isNull()) return mask;
  for (JsonVariantConst script : scripts) {
    const char* familyTag = script.as<const char*>();
    if (!familyTag) continue;
    const size_t groupIndex = findScriptGroupIndex(groupTags, familyTag);
    if (groupIndex < Limits::kMaxScriptGroups) {
      mask |= uint32_t{1} << groupIndex;
    }
  }
  return mask;
}

bool planReplaceScriptGroups(JsonArrayConst groupsArr, std::vector<std::string>& outTags,
                             std::vector<std::string>& outLabels) {
  outTags.clear();
  outLabels.clear();
  if (groupsArr.isNull()) return true;
  const size_t groupCount = std::min(groupsArr.size(), Limits::kMaxScriptGroups);
  outTags.reserve(groupCount);
  outLabels.reserve(groupCount);
  for (size_t groupIndex = 0; groupIndex < groupCount; groupIndex++) {
    JsonObjectConst groupObj = groupsArr[groupIndex].as<JsonObjectConst>();
    const char* tag = groupObj["tag"] | "";
    const char* label = groupObj["label"] | "";
    if (*tag == '\0' || *label == '\0') {
      LOG_ERR("FONT", "Malformed script group at index %zu", groupIndex);
      return false;
    }
    outTags.emplace_back(tag);
    outLabels.emplace_back(label);
  }
  return true;
}

bool planMergeScriptGroups(JsonArrayConst groupsArr, const Catalog& catalog, std::vector<std::string>& outTags,
                           std::vector<std::string>& outLabels) {
  outTags.clear();
  outLabels.clear();
  if (groupsArr.isNull()) return true;
  size_t projectedCount = catalog.scriptGroupTags.size();
  for (size_t groupIndex = 0; groupIndex < groupsArr.size(); groupIndex++) {
    JsonObjectConst groupObj = groupsArr[groupIndex].as<JsonObjectConst>();
    const char* tag = groupObj["tag"] | "";
    const char* label = groupObj["label"] | "";
    if (*tag == '\0' || *label == '\0') {
      LOG_ERR("FONT", "Malformed script group at index %zu", groupIndex);
      return false;
    }
    if (findScriptGroupIndex(catalog.scriptGroupTags, tag) < catalog.scriptGroupTags.size()) {
      continue;
    }
    if (findScriptGroupIndex(outTags, tag) < outTags.size()) {
      continue;
    }
    if (projectedCount >= Limits::kMaxScriptGroups) {
      continue;
    }
    outTags.emplace_back(tag);
    outLabels.emplace_back(label);
    projectedCount++;
  }
  return true;
}

bool familyNameExists(const Catalog& catalog, const std::string& name) {
  return std::any_of(catalog.families.begin(), catalog.families.end(),
                     [&name](const Family& existing) { return existing.name == name; });
}

int findFamilyIndex(const Catalog& catalog, const std::string& name) {
  for (size_t index = 0; index < catalog.families.size(); index++) {
    if (catalog.families[index].name == name) return static_cast<int>(index);
  }
  return -1;
}

void removeFamilyAt(Catalog& catalog, size_t familyIndex) {
  if (familyIndex >= catalog.families.size()) return;
  const Family& victim = catalog.families[familyIndex];
  const size_t removeStart = victim.fileOffset;
  const size_t removeCount = victim.fileCount;
  catalog.files.erase(catalog.files.begin() + static_cast<ptrdiff_t>(removeStart),
                      catalog.files.begin() + static_cast<ptrdiff_t>(removeStart + removeCount));
  catalog.families.erase(catalog.families.begin() + static_cast<ptrdiff_t>(familyIndex));
  for (Family& family : catalog.families) {
    if (family.fileOffset > removeStart) {
      family.fileOffset -= removeCount;
    }
  }
}

bool resolveBaseUrlIndex(const Catalog& catalog, const std::string& baseUrl, ManifestBundleSource source,
                         uint8_t& outIndex, bool& outAddsNew) {
  outAddsNew = false;
  for (size_t index = 0; index < catalog.baseUrls.size(); index++) {
    if (catalog.baseUrls[index] == baseUrl) {
      if (catalog.baseUrlSources[index] != source) {
        LOG_ERR("FONT", "Conflicting manifest bundle source for baseUrl");
        return false;
      }
      outIndex = static_cast<uint8_t>(index);
      return true;
    }
  }
  if (catalog.baseUrls.size() >= Limits::kMaxBaseUrls) {
    LOG_ERR("FONT", "Too many distinct manifest baseUrl values");
    return false;
  }
  outAddsNew = true;
  outIndex = static_cast<uint8_t>(catalog.baseUrls.size());
  return true;
}

void commitBaseUrl(Catalog& catalog, const std::string& baseUrl, ManifestBundleSource source, uint8_t& outIndex) {
  for (size_t index = 0; index < catalog.baseUrls.size(); index++) {
    if (catalog.baseUrls[index] == baseUrl) {
      outIndex = static_cast<uint8_t>(index);
      return;
    }
  }
  catalog.baseUrls.push_back(baseUrl);
  catalog.baseUrlSources.push_back(source);
  outIndex = static_cast<uint8_t>(catalog.baseUrls.size() - 1);
}

struct IngestPlan {
  size_t familiesToAdd = 0;
  size_t filesToAdd = 0;
};

bool planFamilies(JsonArrayConst familiesArr, IngestPlan& plan) {
  for (JsonObjectConst familyObject : familiesArr) {
    const JsonArrayConst familyFiles = familyObject["files"].as<JsonArrayConst>();
    if (familyFiles.isNull() || familyFiles.size() == 0 || familyFiles.size() > Limits::kMaxFilesPerFamily) {
      return false;
    }
    plan.familiesToAdd++;
    plan.filesToAdd += familyFiles.size();
  }
  return true;
}

bool countAfterIngest(const Catalog& scratch, JsonArrayConst familiesArr, IngestMode mode, size_t& outFamilies,
                      size_t& outFiles) {
  outFamilies = scratch.families.size();
  outFiles = scratch.files.size();
  for (JsonObjectConst familyObject : familiesArr) {
    const char* familyName = familyObject["name"] | "";
    const JsonArrayConst familyFiles = familyObject["files"].as<JsonArrayConst>();
    if (familyFiles.isNull() || familyFiles.size() == 0) return false;
    const int existingIndex = findFamilyIndex(scratch, familyName);
    const bool exists = existingIndex >= 0;
    if (mode == IngestMode::MergeReplaceDuplicateNames && exists) {
      outFiles -= scratch.families[static_cast<size_t>(existingIndex)].fileCount;
    }
    if (!exists) {
      outFamilies++;
    }
    outFiles += familyFiles.size();
  }
  return true;
}

bool parseFamilyFromJson(JsonObjectConst fObj, const std::vector<std::string>& scriptGroupTags, uint8_t baseUrlIndex,
                         Family& family, std::vector<File>& pendingFiles) {
  family = Family{};
  pendingFiles.clear();
  family.name = fObj["name"] | "";
  family.description = fObj["description"] | "";
  family.baseUrlIndex = baseUrlIndex;
  if (!isValidFamilyName(family.name.c_str()) || family.name.size() > Limits::kMaxFamilyNameBytes ||
      family.description.empty() || family.description.size() > Limits::kMaxDescriptionBytes) {
    LOG_ERR("FONT", "Malformed manifest family name: %s", family.name.c_str());
    return false;
  }

  const JsonArrayConst familyFiles = fObj["files"].as<JsonArrayConst>();
  if (familyFiles.isNull() || familyFiles.size() == 0 || familyFiles.size() > Limits::kMaxFilesPerFamily) {
    LOG_ERR("FONT", "Manifest has invalid file count");
    return false;
  }

  pendingFiles.reserve(familyFiles.size());
  family.scriptMask = scriptMaskForTags(scriptGroupTags, fObj["scripts"].as<JsonArrayConst>());
  family.totalSize = 0;

  for (JsonObjectConst fileObj : familyFiles) {
    File file;
    const char* fileName = fileObj["name"] | "";
    file.size = fileObj["size"] | 0;

    if (!isValidCpfontFilename(fileName) || !parsePointSize(family.name.c_str(), fileName, file.pointSize) ||
        file.size == 0 || file.size >= Limits::kMaxFontFileBytes || !fileObj["crc32"].is<uint32_t>()) {
      LOG_ERR("FONT", "Malformed manifest file entry: %s", fileName);
      return false;
    }
    const bool duplicatePointSize =
        std::any_of(pendingFiles.begin(), pendingFiles.end(),
                    [&file](const File& existing) { return existing.pointSize == file.pointSize; });
    if (duplicatePointSize) {
      LOG_ERR("FONT", "Duplicate manifest point size: %s", fileName);
      return false;
    }
    file.crc32 = fileObj["crc32"].as<uint32_t>();
    family.totalSize += file.size;
    pendingFiles.push_back(file);
  }
  return true;
}

struct FamilyPlan {
  size_t insertIndex = 0;
  bool replaceExisting = false;
  Family family;
  std::vector<File> files;
};

bool buildFamilyPlans(JsonArrayConst familiesArr, const Catalog& catalog, IngestMode mode, uint8_t baseUrlIndex,
                      const std::vector<std::string>& scriptGroupTags, std::vector<FamilyPlan>& outPlans) {
  outPlans.clear();
  std::vector<std::string> order;
  order.reserve(catalog.families.size() + familiesArr.size());
  for (const Family& existing : catalog.families) {
    order.push_back(existing.name);
  }

  for (JsonObjectConst fObj : familiesArr) {
    const char* familyName = fObj["name"] | "";

    FamilyPlan plan;
    plan.replaceExisting = false;
    plan.insertIndex = order.size();

    const auto orderIt = std::find(order.begin(), order.end(), familyName);
    if (mode == IngestMode::MergeReplaceDuplicateNames && orderIt != order.end()) {
      plan.insertIndex = static_cast<size_t>(orderIt - order.begin());
      plan.replaceExisting = true;
      order.erase(orderIt);
    } else if (mode == IngestMode::Replace && orderIt != order.end()) {
      LOG_ERR("FONT", "Duplicate manifest family name: %s", familyName);
      return false;
    }

    if (!parseFamilyFromJson(fObj, scriptGroupTags, baseUrlIndex, plan.family, plan.files)) {
      return false;
    }

    order.insert(order.begin() + static_cast<ptrdiff_t>(plan.insertIndex), plan.family.name);
    outPlans.push_back(std::move(plan));
  }
  return true;
}

void applyFamilyPlans(Catalog& catalog, std::vector<FamilyPlan>& plans) {
  for (FamilyPlan& plan : plans) {
    if (plan.replaceExisting) {
      assert(plan.insertIndex < catalog.families.size());
      removeFamilyAt(catalog, plan.insertIndex);
    } else {
      assert(plan.insertIndex <= catalog.families.size());
    }
    plan.family.fileOffset = catalog.files.size();
    plan.family.fileCount = plan.files.size();
    catalog.files.insert(catalog.files.end(), plan.files.begin(), plan.files.end());
    catalog.families.insert(catalog.families.begin() + static_cast<ptrdiff_t>(plan.insertIndex),
                            std::move(plan.family));
  }
}

IngestResult applyDocumentToCatalog(const JsonDocument& doc, Catalog& catalog, IngestMode mode, const char* sourceUrl,
                                    ManifestBundleSource bundleSource) {
  const int version = doc["version"] | 0;
  if (version != kExpectedManifestSchemaVersion) {
    LOG_ERR("FONT", "Unsupported manifest version: %d", version);
    return IngestResult::Version;
  }

  const char* baseUrlCStr = doc["baseUrl"] | "";
  const std::string baseUrl = baseUrlCStr;
  if (baseUrl.empty() || baseUrl.size() > Limits::kMaxBaseUrlBytes || baseUrl.rfind("https://", 0) != 0 ||
      baseUrl.back() != '/') {
    LOG_ERR("FONT", "Manifest has invalid baseUrl");
    return IngestResult::Parse;
  }

  const JsonArrayConst groupsArr = doc["scriptGroups"];
  const JsonArrayConst familiesArr = doc["families"];
  if (familiesArr.isNull() || familiesArr.size() == 0) {
    LOG_ERR("FONT", "Manifest has invalid family count from %s", sourceUrl);
    return IngestResult::Parse;
  }

  Catalog mergeScratch;
  const bool restoreCatalogOnFailure = mode != IngestMode::Replace;
  if (restoreCatalogOnFailure) {
    mergeScratch = std::move(catalog);
  }
  const auto failIngest = [&](IngestResult result) -> IngestResult {
    if (restoreCatalogOnFailure) {
      catalog = std::move(mergeScratch);
    }
    return result;
  };

  const Catalog& validateBase = restoreCatalogOnFailure ? mergeScratch : kEmptyCatalog;

  std::vector<std::string> plannedGroupTags;
  std::vector<std::string> plannedGroupLabels;
  if (mode == IngestMode::Replace) {
    if (!planReplaceScriptGroups(groupsArr, plannedGroupTags, plannedGroupLabels)) {
      return failIngest(IngestResult::Parse);
    }
  } else if (!planMergeScriptGroups(groupsArr, validateBase, plannedGroupTags, plannedGroupLabels)) {
    return failIngest(IngestResult::Parse);
  }

  std::vector<std::string> effectiveScriptTags = validateBase.scriptGroupTags;
  if (mode == IngestMode::Replace) {
    effectiveScriptTags = plannedGroupTags;
  } else {
    effectiveScriptTags.reserve(effectiveScriptTags.size() + plannedGroupTags.size());
    effectiveScriptTags.insert(effectiveScriptTags.end(), plannedGroupTags.begin(), plannedGroupTags.end());
  }

  IngestPlan plan;
  if (!planFamilies(familiesArr, plan)) {
    LOG_ERR("FONT", "Manifest has invalid file count from %s", sourceUrl);
    return failIngest(IngestResult::Parse);
  }

  size_t finalFamilies = 0;
  size_t finalFiles = 0;
  if (mode == IngestMode::Replace) {
    finalFamilies = plan.familiesToAdd;
    finalFiles = plan.filesToAdd;
  } else if (!countAfterIngest(validateBase, familiesArr, mode, finalFamilies, finalFiles)) {
    LOG_ERR("FONT", "Manifest has invalid file count from %s", sourceUrl);
    return failIngest(IngestResult::Parse);
  }

  if (finalFamilies > Limits::kMaxManifestFamilies) {
    LOG_ERR("FONT", "Manifest from %s would exceed %zu families (%zu)", sourceUrl, Limits::kMaxManifestFamilies,
            finalFamilies);
    return failIngest(IngestResult::Overflow);
  }
  if (finalFiles > Limits::kMaxManifestFiles) {
    LOG_ERR("FONT", "Manifest from %s would exceed %zu files (%zu)", sourceUrl, Limits::kMaxManifestFiles, finalFiles);
    return failIngest(IngestResult::Overflow);
  }

  uint8_t baseUrlIndex = 0;
  bool addsBaseUrl = false;
  if (!resolveBaseUrlIndex(validateBase, baseUrl, bundleSource, baseUrlIndex, addsBaseUrl)) {
    return failIngest(IngestResult::Parse);
  }

  std::vector<FamilyPlan> familyPlans;
  if (!buildFamilyPlans(familiesArr, validateBase, mode, baseUrlIndex, effectiveScriptTags, familyPlans)) {
    return failIngest(IngestResult::Parse);
  }

  Catalog scratch;
  if (mode == IngestMode::Replace) {
    scratch = Catalog{};
    scratch.scriptGroupTags.reserve(plannedGroupTags.size());
    scratch.scriptGroupLabels.reserve(plannedGroupLabels.size());
    if (!groupsArr.isNull() && groupsArr.size() > Limits::kMaxScriptGroups) {
      LOG_ERR("FONT", "Manifest declares more than %zu script groups; extra groups ignored", Limits::kMaxScriptGroups);
    }
    scratch.scriptGroupTags = plannedGroupTags;
    scratch.scriptGroupLabels = plannedGroupLabels;
  } else {
    scratch = std::move(mergeScratch);
    scratch.scriptGroupTags.reserve(scratch.scriptGroupTags.size() + plannedGroupTags.size());
    scratch.scriptGroupLabels.reserve(scratch.scriptGroupLabels.size() + plannedGroupLabels.size());
    scratch.scriptGroupTags.insert(scratch.scriptGroupTags.end(), plannedGroupTags.begin(), plannedGroupTags.end());
    scratch.scriptGroupLabels.insert(scratch.scriptGroupLabels.end(), plannedGroupLabels.begin(),
                                     plannedGroupLabels.end());
  }

  if (addsBaseUrl) {
    commitBaseUrl(scratch, baseUrl, bundleSource, baseUrlIndex);
  }

  scratch.families.reserve(finalFamilies);
  scratch.files.reserve(finalFiles);
  scratch.baseUrls.reserve(scratch.baseUrls.size() + (addsBaseUrl ? 1u : 0u));

  applyFamilyPlans(scratch, familyPlans);

  catalog = std::move(scratch);
  return IngestResult::Ok;
}

}  // namespace

bool parsePointSize(const char* familyName, const char* fileName, uint8_t& pointSize) {
  const size_t familyLength = strlen(familyName);
  const size_t fileNameLength = strlen(fileName);
  if (fileNameLength <= familyLength + 1) return false;
  if (strncmp(fileName, familyName, familyLength) != 0 || fileName[familyLength] != '_') return false;

  const char* cursor = fileName + familyLength + 1;
  if (*cursor < '1' || *cursor > '9') return false;

  uint16_t value = 0;
  while (std::isdigit(static_cast<unsigned char>(*cursor))) {
    value = static_cast<uint16_t>(value * 10 + (*cursor - '0'));
    if (value > UINT8_MAX) return false;
    ++cursor;
  }
  if (strcmp(cursor, ".cpfont") != 0) return false;

  pointSize = static_cast<uint8_t>(value);
  return true;
}

IngestResult ingestDocument(const JsonDocument& doc, Catalog& catalog, IngestMode mode, const char* sourceUrl,
                            ManifestBundleSource bundleSource) {
  if (sourceUrl == nullptr) sourceUrl = "(unknown)";
  return applyDocumentToCatalog(doc, catalog, mode, sourceUrl, bundleSource);
}

void applyDefaultScriptGroupForUnscriptedFamilies(Catalog& catalog) {
  if (catalog.families.empty()) return;

  size_t cjkIndex = findScriptGroupIndex(catalog.scriptGroupTags, kCjkScriptGroupTag);
  bool addedCjkGroup = false;
  if (cjkIndex >= catalog.scriptGroupTags.size()) {
    if (catalog.scriptGroupTags.size() >= Limits::kMaxScriptGroups) {
      LOG_ERR("FONT", "Cannot add default CJK script group; catalog full");
      return;
    }
    catalog.scriptGroupTags.emplace_back(kCjkScriptGroupTag);
    catalog.scriptGroupLabels.emplace_back("");
    cjkIndex = catalog.scriptGroupTags.size() - 1;
    addedCjkGroup = true;
  }

  const uint32_t cjkBit = uint32_t{1} << cjkIndex;
  bool appliedCjkDefault = false;
  for (Family& family : catalog.families) {
    if (family.scriptMask != 0) continue;
    if (!isCrossMuxBundleSource(catalog, family.baseUrlIndex)) {
      LOG_ERR("FONT", "Unscripted family %s has no script group; listing under All fonts", family.name.c_str());
      continue;
    }
    family.scriptMask = cjkBit;
    appliedCjkDefault = true;
  }

  if (addedCjkGroup && !appliedCjkDefault) {
    catalog.scriptGroupTags.pop_back();
    catalog.scriptGroupLabels.pop_back();
  }
}

size_t estimateCatalogHeapBytes(const Catalog& catalog) {
  size_t bytes = catalog.baseUrls.capacity() * sizeof(std::string);
  for (const auto& baseUrl : catalog.baseUrls) {
    bytes += baseUrl.capacity();
  }
  bytes += catalog.baseUrlSources.capacity() * sizeof(ManifestBundleSource);
  bytes += catalog.scriptGroupTags.capacity() * sizeof(std::string);
  for (const auto& tag : catalog.scriptGroupTags) {
    bytes += tag.capacity();
  }
  bytes += catalog.scriptGroupLabels.capacity() * sizeof(std::string);
  for (const auto& label : catalog.scriptGroupLabels) {
    bytes += label.capacity();
  }
  bytes += catalog.families.capacity() * sizeof(Family);
  for (const auto& family : catalog.families) {
    bytes += family.name.capacity() + family.description.capacity();
  }
  bytes += catalog.files.capacity() * sizeof(File);
  return bytes;
}

}  // namespace FontManifestCatalog
