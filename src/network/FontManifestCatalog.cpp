#include "FontManifestCatalog.h"

#include <Logging.h>

#include <algorithm>
#include <cctype>
#include <cstring>

namespace FontManifestCatalog {

namespace {

constexpr int kExpectedManifestSchemaVersion = 1;
constexpr char kDefaultCjkScriptTag[] = "cjk";
constexpr char kDefaultCjkScriptLabel[] = "CJK";

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

bool mergeScriptGroups(JsonArrayConst groupsArr, Catalog& catalog) {
  if (groupsArr.isNull()) return true;
  const size_t incomingCount = groupsArr.size();
  for (size_t groupIndex = 0; groupIndex < incomingCount; groupIndex++) {
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
    if (catalog.scriptGroupTags.size() >= Limits::kMaxScriptGroups) {
      LOG_ERR("FONT", "Manifest declares more than %zu script groups; extra groups ignored", Limits::kMaxScriptGroups);
      continue;
    }
    catalog.scriptGroupTags.emplace_back(tag);
    catalog.scriptGroupLabels.emplace_back(label);
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

bool internBaseUrl(Catalog& catalog, const std::string& baseUrl, uint8_t& outIndex) {
  for (size_t index = 0; index < catalog.baseUrls.size(); index++) {
    if (catalog.baseUrls[index] == baseUrl) {
      outIndex = static_cast<uint8_t>(index);
      return true;
    }
  }
  if (catalog.baseUrls.size() >= Limits::kMaxBaseUrls) {
    LOG_ERR("FONT", "Too many distinct manifest baseUrl values");
    return false;
  }
  catalog.baseUrls.push_back(baseUrl);
  outIndex = static_cast<uint8_t>(catalog.baseUrls.size() - 1);
  return true;
}

struct IngestPlan {
  size_t familiesToAdd = 0;
  size_t filesToAdd = 0;
};

bool planFamilies(JsonArrayConst familiesArr, const Catalog& catalog, IngestMode mode, IngestPlan& plan) {
  for (JsonObjectConst familyObject : familiesArr) {
    const char* familyName = familyObject["name"] | "";
    const JsonArrayConst familyFiles = familyObject["files"].as<JsonArrayConst>();
    if (familyFiles.isNull() || familyFiles.size() == 0 || familyFiles.size() > Limits::kMaxFilesPerFamily) {
      return false;
    }
    const bool exists = familyNameExists(catalog, familyName);
    if (mode == IngestMode::MergeSkipDuplicateNames && exists) {
      continue;
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
    if (mode == IngestMode::MergeSkipDuplicateNames && exists) {
      continue;
    }
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

bool ingestFamilies(JsonArrayConst familiesArr, Catalog& catalog, IngestMode mode, uint8_t baseUrlIndex) {
  for (JsonObjectConst fObj : familiesArr) {
    const char* familyName = fObj["name"] | "";
    if (mode == IngestMode::MergeSkipDuplicateNames && familyNameExists(catalog, familyName)) {
      continue;
    }
    if (mode == IngestMode::MergeReplaceDuplicateNames) {
      const int existingIndex = findFamilyIndex(catalog, familyName);
      if (existingIndex >= 0) {
        removeFamilyAt(catalog, static_cast<size_t>(existingIndex));
      }
    }

    Family family;
    family.name = familyName;
    family.description = fObj["description"] | "";
    family.baseUrlIndex = baseUrlIndex;
    if (!isValidFamilyName(family.name.c_str()) || family.name.size() > Limits::kMaxFamilyNameBytes ||
        family.description.empty() || family.description.size() > Limits::kMaxDescriptionBytes ||
        familyNameExists(catalog, family.name)) {
      LOG_ERR("FONT", "Malformed manifest family name: %s", family.name.c_str());
      return false;
    }

    const JsonArrayConst familyFiles = fObj["files"].as<JsonArrayConst>();
    if (familyFiles.isNull() || familyFiles.size() == 0 || familyFiles.size() > Limits::kMaxFilesPerFamily) {
      LOG_ERR("FONT", "Manifest has invalid file count");
      return false;
    }

    std::vector<File> pendingFiles;
    pendingFiles.reserve(familyFiles.size());
    family.scriptMask = scriptMaskForTags(catalog.scriptGroupTags, fObj["scripts"].as<JsonArrayConst>());
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

    family.fileOffset = catalog.files.size();
    family.fileCount = pendingFiles.size();
    catalog.files.insert(catalog.files.end(), pendingFiles.begin(), pendingFiles.end());
    catalog.families.push_back(std::move(family));
  }

  return true;
}

IngestResult applyDocumentToCatalog(const JsonDocument& doc, Catalog& catalog, IngestMode mode, const char* sourceUrl) {
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

  Catalog scratch = catalog;
  if (mode == IngestMode::Replace) {
    scratch = Catalog{};
  }

  if (mode == IngestMode::Replace) {
    scratch.scriptGroupTags.clear();
    scratch.scriptGroupLabels.clear();
    scratch.families.clear();
    scratch.files.clear();
    scratch.baseUrls.clear();
    if (!groupsArr.isNull()) {
      const size_t groupCount = std::min(groupsArr.size(), Limits::kMaxScriptGroups);
      scratch.scriptGroupTags.reserve(groupCount);
      scratch.scriptGroupLabels.reserve(groupCount);
      if (groupsArr.size() > Limits::kMaxScriptGroups) {
        LOG_ERR("FONT", "Manifest declares more than %zu script groups; extra groups ignored",
                Limits::kMaxScriptGroups);
      }
      for (size_t groupIndex = 0; groupIndex < groupCount; groupIndex++) {
        JsonObjectConst groupObj = groupsArr[groupIndex].as<JsonObjectConst>();
        const char* tag = groupObj["tag"] | "";
        const char* label = groupObj["label"] | "";
        if (*tag == '\0' || *label == '\0') {
          LOG_ERR("FONT", "Malformed script group at index %zu", groupIndex);
          return IngestResult::Parse;
        }
        scratch.scriptGroupTags.emplace_back(tag);
        scratch.scriptGroupLabels.emplace_back(label);
      }
    }
  } else if (!mergeScriptGroups(groupsArr, scratch)) {
    return IngestResult::Parse;
  }

  IngestPlan plan;
  if (!planFamilies(familiesArr, scratch, mode, plan)) {
    LOG_ERR("FONT", "Manifest has invalid file count from %s", sourceUrl);
    return IngestResult::Parse;
  }

  size_t finalFamilies = 0;
  size_t finalFiles = 0;
  if (mode == IngestMode::Replace) {
    finalFamilies = plan.familiesToAdd;
    finalFiles = plan.filesToAdd;
  } else if (!countAfterIngest(scratch, familiesArr, mode, finalFamilies, finalFiles)) {
    LOG_ERR("FONT", "Manifest has invalid file count from %s", sourceUrl);
    return IngestResult::Parse;
  }

  if (finalFamilies > Limits::kMaxManifestFamilies) {
    LOG_ERR("FONT", "Manifest from %s would exceed %zu families (%zu)", sourceUrl, Limits::kMaxManifestFamilies,
            finalFamilies);
    return IngestResult::Overflow;
  }
  if (finalFiles > Limits::kMaxManifestFiles) {
    LOG_ERR("FONT", "Manifest from %s would exceed %zu files (%zu)", sourceUrl, Limits::kMaxManifestFiles, finalFiles);
    return IngestResult::Overflow;
  }

  uint8_t baseUrlIndex = 0;
  if (!internBaseUrl(scratch, baseUrl, baseUrlIndex)) {
    return IngestResult::Parse;
  }

  if (!ingestFamilies(familiesArr, scratch, mode, baseUrlIndex)) {
    return IngestResult::Parse;
  }

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

IngestResult ingestDocument(const JsonDocument& doc, Catalog& catalog, IngestMode mode, const char* sourceUrl) {
  if (sourceUrl == nullptr) sourceUrl = "(unknown)";
  return applyDocumentToCatalog(doc, catalog, mode, sourceUrl);
}

void applyDefaultScriptGroupForUnscriptedFamilies(Catalog& catalog) {
  if (catalog.families.empty()) return;

  size_t cjkIndex = findScriptGroupIndex(catalog.scriptGroupTags, kDefaultCjkScriptTag);
  if (cjkIndex >= catalog.scriptGroupTags.size()) {
    if (catalog.scriptGroupTags.size() >= Limits::kMaxScriptGroups) {
      LOG_ERR("FONT", "Cannot add default CJK script group; catalog full");
      return;
    }
    catalog.scriptGroupTags.emplace_back(kDefaultCjkScriptTag);
    catalog.scriptGroupLabels.emplace_back(kDefaultCjkScriptLabel);
    cjkIndex = catalog.scriptGroupTags.size() - 1;
  }

  const uint32_t cjkBit = uint32_t{1} << cjkIndex;
  for (Family& family : catalog.families) {
    if (family.scriptMask == 0) {
      family.scriptMask = cjkBit;
    }
  }
}

size_t estimateCatalogHeapBytes(const Catalog& catalog) {
  size_t bytes = catalog.baseUrls.capacity() * sizeof(std::string);
  for (const auto& baseUrl : catalog.baseUrls) {
    bytes += baseUrl.capacity();
  }
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
