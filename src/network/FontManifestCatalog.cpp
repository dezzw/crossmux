#include "FontManifestCatalog.h"

#include <Logging.h>

#include <algorithm>
#include <cctype>
#include <cstring>

namespace FontManifestCatalog {

namespace {

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

bool planScriptGroups(JsonArrayConst groupsArr, std::vector<std::string>& outTags,
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

bool familyNameExists(const Catalog& catalog, const std::string& name) {
  return std::any_of(catalog.families.begin(), catalog.families.end(),
                     [&name](const Family& existing) { return existing.name == name; });
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

IngestResult applyDocumentToCatalog(const JsonDocument& doc, Catalog& catalog, const char* sourceUrl) {
  const int version = doc["version"] | 0;
  if (version != kExpectedManifestSchemaVersion) {
    LOG_ERR("FONT", "Unsupported manifest version %d from %s", version, sourceUrl);
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

  std::vector<std::string> scriptGroupTags;
  std::vector<std::string> scriptGroupLabels;
  if (!planScriptGroups(groupsArr, scriptGroupTags, scriptGroupLabels)) {
    return IngestResult::Parse;
  }

  size_t fileCount = 0;
  for (JsonObjectConst familyObject : familiesArr) {
    const JsonArrayConst familyFiles = familyObject["files"].as<JsonArrayConst>();
    if (familyFiles.isNull() || familyFiles.size() == 0 || familyFiles.size() > Limits::kMaxFilesPerFamily) {
      LOG_ERR("FONT", "Manifest has invalid file count from %s", sourceUrl);
      return IngestResult::Parse;
    }
    fileCount += familyFiles.size();
  }

  if (familiesArr.size() > Limits::kMaxManifestFamilies) {
    LOG_ERR("FONT", "Manifest from %s exceeds %zu families (%zu)", sourceUrl, Limits::kMaxManifestFamilies,
            familiesArr.size());
    return IngestResult::Overflow;
  }
  if (fileCount > Limits::kMaxManifestFiles) {
    LOG_ERR("FONT", "Manifest from %s exceeds %zu files (%zu)", sourceUrl, Limits::kMaxManifestFiles, fileCount);
    return IngestResult::Overflow;
  }

  Catalog scratch;
  scratch.scriptGroupTags = std::move(scriptGroupTags);
  scratch.scriptGroupLabels = std::move(scriptGroupLabels);
  scratch.baseUrls.push_back(baseUrl);
  const uint8_t baseUrlIndex = 0;

  scratch.families.reserve(familiesArr.size());
  scratch.files.reserve(fileCount);

  for (JsonObjectConst fObj : familiesArr) {
    const char* familyName = fObj["name"] | "";
    if (familyNameExists(scratch, familyName)) {
      LOG_ERR("FONT", "Duplicate manifest family name: %s", familyName);
      return IngestResult::Parse;
    }

    Family family;
    std::vector<File> pendingFiles;
    if (!parseFamilyFromJson(fObj, scratch.scriptGroupTags, baseUrlIndex, family, pendingFiles)) {
      return IngestResult::Parse;
    }

    family.fileOffset = scratch.files.size();
    family.fileCount = pendingFiles.size();
    scratch.files.insert(scratch.files.end(), pendingFiles.begin(), pendingFiles.end());
    scratch.families.push_back(std::move(family));
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

IngestResult ingestDocument(const JsonDocument& doc, Catalog& catalog, const char* sourceUrl) {
  if (sourceUrl == nullptr) sourceUrl = "(unknown)";
  return applyDocumentToCatalog(doc, catalog, sourceUrl);
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
