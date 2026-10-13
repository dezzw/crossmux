#include "SemVerCompare.h"

#include <cctype>
#include <cstring>

namespace {

struct ParsedVersion {
  int major = 0;
  int minor = 0;
  int patch = 0;
  bool valid = false;
  bool hasPrerelease = false;
  char prerelease[96];
};

bool parseSegment(const char*& cursor, int& out) {
  if (!std::isdigit(static_cast<unsigned char>(*cursor))) return false;
  int value = 0;
  while (std::isdigit(static_cast<unsigned char>(*cursor))) {
    value = value * 10 + (*cursor - '0');
    ++cursor;
  }
  out = value;
  return true;
}

ParsedVersion parseVersion(const char* version) {
  ParsedVersion parsed;
  if (version == nullptr || *version == '\0') return parsed;

  const char* cursor = version;
  if (*cursor == 'v' || *cursor == 'V') ++cursor;
  if (!parseSegment(cursor, parsed.major)) return parsed;
  if (*cursor++ != '.') return parsed;
  if (!parseSegment(cursor, parsed.minor)) return parsed;
  if (*cursor++ != '.') return parsed;
  if (!parseSegment(cursor, parsed.patch)) return parsed;

  if (*cursor == '-') {
    ++cursor;
    parsed.hasPrerelease = true;
    size_t i = 0;
    while (*cursor != '\0' && *cursor != '+' && i + 1 < sizeof(parsed.prerelease)) {
      parsed.prerelease[i++] = *cursor++;
    }
    parsed.prerelease[i] = '\0';
  }

  parsed.valid = true;
  return parsed;
}

bool isNumericIdentifier(const char* start, const char* end) {
  if (start == end) return false;
  for (const char* c = start; c < end; ++c) {
    if (!std::isdigit(static_cast<unsigned char>(*c))) return false;
  }
  return true;
}

int compareIdentifiers(const char* aStart, const char* aEnd, const char* bStart, const char* bEnd) {
  const bool aNum = isNumericIdentifier(aStart, aEnd);
  const bool bNum = isNumericIdentifier(bStart, bEnd);
  if (aNum && bNum) {
    int aVal = 0;
    for (const char* c = aStart; c < aEnd; ++c) {
      aVal = aVal * 10 + (*c - '0');
    }
    int bVal = 0;
    for (const char* c = bStart; c < bEnd; ++c) {
      bVal = bVal * 10 + (*c - '0');
    }
    if (aVal != bVal) return aVal < bVal ? -1 : 1;
    return 0;
  }
  const size_t aLen = static_cast<size_t>(aEnd - aStart);
  const size_t bLen = static_cast<size_t>(bEnd - bStart);
  const size_t minLen = aLen < bLen ? aLen : bLen;
  const int cmp = memcmp(aStart, bStart, minLen);
  if (cmp != 0) return cmp < 0 ? -1 : 1;
  if (aLen != bLen) return aLen < bLen ? -1 : 1;
  return 0;
}

int comparePrerelease(const char* a, const char* b) {
  if (a == nullptr || *a == '\0') {
    return (b == nullptr || *b == '\0') ? 0 : -1;
  }
  if (b == nullptr || *b == '\0') return 1;

  const char* aCursor = a;
  const char* bCursor = b;
  while (true) {
    const char* aStart = aCursor;
    const char* bStart = bCursor;
    while (*aCursor != '\0' && *aCursor != '.') ++aCursor;
    while (*bCursor != '\0' && *bCursor != '.') ++bCursor;
    const int idCmp = compareIdentifiers(aStart, aCursor, bStart, bCursor);
    if (idCmp != 0) return idCmp;
    const bool aDone = *aCursor == '\0';
    const bool bDone = *bCursor == '\0';
    if (aDone && bDone) return 0;
    if (aDone) return -1;
    if (bDone) return 1;
    ++aCursor;
    ++bCursor;
  }
}

}  // namespace

int semverCompareNewer(const char* currentVersion, const char* latestVersion) {
  const ParsedVersion current = parseVersion(currentVersion);
  const ParsedVersion latest = parseVersion(latestVersion);
  if (!current.valid || !latest.valid) return 0;

  if (latest.major != current.major) return latest.major > current.major ? 1 : -1;
  if (latest.minor != current.minor) return latest.minor > current.minor ? 1 : -1;
  if (latest.patch != current.patch) return latest.patch > current.patch ? 1 : -1;

  if (!current.hasPrerelease && latest.hasPrerelease) return -1;
  if (current.hasPrerelease && !latest.hasPrerelease) return 1;

  if (current.hasPrerelease && latest.hasPrerelease) {
    const int pre = comparePrerelease(current.prerelease, latest.prerelease);
    if (pre != 0) return pre > 0 ? 1 : -1;
  }

  return 0;
}
