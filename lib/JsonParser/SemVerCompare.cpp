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
  bool hasBuild = false;
  char build[32];
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
  if (*cursor == '+') {
    ++cursor;
    parsed.hasBuild = true;
    size_t i = 0;
    while (*cursor != '\0' && i + 1 < sizeof(parsed.build)) {
      parsed.build[i++] = *cursor++;
    }
    parsed.build[i] = '\0';
  }

  parsed.valid = true;
  return parsed;
}

int compareAscii(const char* a, const char* b) {
  if (a == nullptr) a = "";
  if (b == nullptr) b = "";
  while (*a != '\0' && *b != '\0') {
    const unsigned char ca = static_cast<unsigned char>(*a);
    const unsigned char cb = static_cast<unsigned char>(*b);
    if (ca != cb) return ca < cb ? -1 : 1;
    ++a;
    ++b;
  }
  if (*a == '\0' && *b == '\0') return 0;
  return *a == '\0' ? -1 : 1;
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
    const int pre = compareAscii(current.prerelease, latest.prerelease);
    if (pre != 0) return pre < 0 ? 1 : -1;
    if (current.hasBuild && latest.hasBuild) {
      const int buildCmp = compareAscii(current.build, latest.build);
      if (buildCmp != 0) return buildCmp < 0 ? 1 : -1;
    } else if (!current.hasBuild && latest.hasBuild) {
      return 1;
    } else if (current.hasBuild && !latest.hasBuild) {
      return -1;
    }
  }

  return 0;
}
