#include "FontManifestCatalog.h"

#include <Logging.h>

#include <algorithm>
#include <cctype>
#include <cstring>

namespace FontManifestCatalog {

namespace {

// Must stay in sync with FONTS_MANIFEST_VERSION in FontDownloadActivity.h.
constexpr int kExpectedManifestSchemaVersion = 1;

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

bool ingestFamilies(JsonArrayConst familiesArr, Catalog& catalog, IngestMode mode, const std::string& baseUrl) {
  size_t incomingFileCount = 0;
  for (JsonObjectConst familyObject : familiesArr) {
    const char* familyName = familyObject["name"] | "";
    if (mode == IngestMode::MergeSkipDuplicateNames && familyNameExists(catalog, familyName)) {
      continue;
    }
    const JsonArrayConst familyFiles = familyObject["files"].as<JsonArrayConst>();
    if (familyFiles.isNull() || familyFiles.size() == 0 || familyFiles.size() > Limits::kMaxFilesPerFamily) {
      LOG_ERR("FONT", "Manifest has invalid file count");
      return false;
    }
    incomingFileCount += familyFiles.size();
  }
  if (catalog.files.size() + incomingFileCount > Limits::kMaxManifestFiles) {
    LOG_ERR("FONT", "Manifest has invalid file count");
    return false;
  }

  if (mode == IngestMode::Replace) {
    catalog.families.clear();
    catalog.files.clear();
  }

  catalog.families.reserve(catalog.families.size() + familiesArr.size());
  catalog.files.reserve(catalog.files.size() + incomingFileCount);

  for (JsonObjectConst fObj : familiesArr) {
    Family family;
    family.name = fObj["name"] | "";
    family.description = fObj["description"] | "";
    family.baseUrl = baseUrl;
    if (mode == IngestMode::MergeSkipDuplicateNames && familyNameExists(catalog, family.name)) {
      continue;
    }
    const bool duplicateFamily = familyNameExists(catalog, family.name);
    if (!isValidFamilyName(family.name.c_str()) || family.name.size() > Limits::kMaxFamilyNameBytes ||
        family.description.empty() || family.description.size() > Limits::kMaxDescriptionBytes || duplicateFamily) {
      LOG_ERR("FONT", "Malformed manifest family name: %s", family.name.c_str());
      return false;
    }

    const JsonArrayConst familyFiles = fObj["files"].as<JsonArrayConst>();
    family.fileOffset = catalog.files.size();
    family.fileCount = familyFiles.size();
    family.scriptMask = scriptMaskForTags(catalog.scriptGroupTags, fObj["scripts"].as<JsonArrayConst>());
    family.totalSize = 0;
    family.installed = false;

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
          std::any_of(catalog.files.begin() + static_cast<ptrdiff_t>(family.fileOffset), catalog.files.end(),
                      [&file](const File& existing) { return existing.pointSize == file.pointSize; });
      if (duplicatePointSize) {
        LOG_ERR("FONT", "Duplicate manifest point size: %s", fileName);
        return false;
      }
      file.crc32 = fileObj["crc32"].as<uint32_t>();

      family.totalSize += file.size;
      catalog.files.push_back(file);
    }

    catalog.families.push_back(std::move(family));
  }

  return true;
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

IngestResult ingestDocument(const JsonDocument& doc, Catalog& catalog, IngestMode mode) {
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
  if (mode == IngestMode::Replace) {
    catalog.scriptGroupTags.clear();
    catalog.scriptGroupLabels.clear();
    if (groupsArr.isNull()) {
      LOG_ERR("FONT", "Manifest has invalid scriptGroups");
      return IngestResult::Parse;
    }
    const size_t groupCount = std::min(groupsArr.size(), Limits::kMaxScriptGroups);
    catalog.scriptGroupTags.reserve(groupCount);
    catalog.scriptGroupLabels.reserve(groupCount);
    if (groupsArr.size() > Limits::kMaxScriptGroups) {
      LOG_ERR("FONT", "Manifest declares more than %zu script groups; extra groups ignored", Limits::kMaxScriptGroups);
    }
    for (size_t groupIndex = 0; groupIndex < groupCount; groupIndex++) {
      JsonObjectConst groupObj = groupsArr[groupIndex].as<JsonObjectConst>();
      const char* tag = groupObj["tag"] | "";
      const char* label = groupObj["label"] | "";
      if (*tag == '\0' || *label == '\0') {
        LOG_ERR("FONT", "Malformed script group at index %zu", groupIndex);
        return IngestResult::Parse;
      }
      catalog.scriptGroupTags.emplace_back(tag);
      catalog.scriptGroupLabels.emplace_back(label);
    }
  } else if (!mergeScriptGroups(groupsArr, catalog)) {
    return IngestResult::Parse;
  }

  const JsonArrayConst familiesArr = doc["families"];
  if (familiesArr.isNull()) {
    LOG_ERR("FONT", "Manifest has invalid family count: 0");
    return IngestResult::Parse;
  }
  if (familiesArr.size() == 0) {
    LOG_ERR("FONT", "Manifest has invalid family count: %zu", familiesArr.size());
    return IngestResult::Parse;
  }
  if (mode == IngestMode::Replace && (familiesArr.size() > Limits::kMaxManifestFamilies ||
                                      catalog.families.size() + familiesArr.size() > Limits::kMaxManifestFamilies)) {
    LOG_ERR("FONT", "Manifest has invalid family count: %zu", familiesArr.size());
    return IngestResult::Parse;
  }
  if (!ingestFamilies(familiesArr, catalog, mode, baseUrl)) {
    return IngestResult::Parse;
  }
  if (catalog.families.size() > Limits::kMaxManifestFamilies) {
    LOG_ERR("FONT", "Merged manifest would exceed %zu families", Limits::kMaxManifestFamilies);
    return IngestResult::Parse;
  }

  return IngestResult::Ok;
}

size_t estimateCatalogHeapBytes(const Catalog& catalog) {
  size_t bytes = catalog.scriptGroupTags.capacity() * sizeof(std::string);
  for (const auto& tag : catalog.scriptGroupTags) {
    bytes += tag.capacity();
  }
  bytes += catalog.scriptGroupLabels.capacity() * sizeof(std::string);
  for (const auto& label : catalog.scriptGroupLabels) {
    bytes += label.capacity();
  }
  bytes += catalog.families.capacity() * sizeof(Family);
  for (const auto& family : catalog.families) {
    bytes += family.name.capacity() + family.description.capacity() + family.baseUrl.capacity();
  }
  bytes += catalog.files.capacity() * sizeof(File);
  return bytes;
}

}  // namespace FontManifestCatalog
