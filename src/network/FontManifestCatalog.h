#pragma once

#include <ArduinoJson.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace FontManifestCatalog {

inline constexpr char kCjkScriptGroupTag[] = "cjk";

struct Limits {
  static constexpr size_t kMaxManifestBytes = 96 * 1024;
  static constexpr size_t kMaxManifestFamilies = 64;
  static constexpr size_t kMaxManifestFiles = 256;
  static constexpr size_t kMaxFilesPerFamily = 16;
  static constexpr size_t kMaxFamilyNameBytes = 64;
  static constexpr size_t kMaxDescriptionBytes = 160;
  static constexpr size_t kMaxBaseUrlBytes = 256;
  static constexpr size_t kMaxFontFileBytes = 25 * 1024 * 1024;
  static constexpr size_t kMaxScriptGroups = 32;
  static constexpr size_t kMaxBaseUrls = 4;
};

struct File {
  size_t size = 0;
  uint32_t crc32 = 0;
  uint8_t pointSize = 0;
};

struct Family {
  std::string name;
  std::string description;
  uint8_t baseUrlIndex = 0;
  size_t fileOffset = 0;
  size_t fileCount = 0;
  size_t totalSize = 0;
  bool installed = false;
  bool hasUpdate = false;
  uint32_t scriptMask = 0;
};

struct Catalog {
  std::vector<std::string> baseUrls;
  std::vector<std::string> scriptGroupTags;
  std::vector<std::string> scriptGroupLabels;
  std::vector<Family> families;
  std::vector<File> files;
};

enum class IngestResult { Ok, Parse, Version, Overflow };

bool parsePointSize(const char* familyName, const char* fileName, uint8_t& pointSize);

IngestResult ingestDocument(const JsonDocument& doc, Catalog& catalog, const char* sourceUrl);

void applyDefaultScriptGroupForUnscriptedFamilies(Catalog& catalog);

inline const char* baseUrlForFamily(const Catalog& catalog, const Family& family) {
  if (family.baseUrlIndex >= catalog.baseUrls.size()) return "";
  return catalog.baseUrls[family.baseUrlIndex].c_str();
}

size_t estimateCatalogHeapBytes(const Catalog& catalog);

}  // namespace FontManifestCatalog
