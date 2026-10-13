#include "SemVerCompare.h"

#include <cctype>
#include <cstring>

namespace {

struct ParsedVersion {
  int major = 0;
  int minor = 0;
  int patch = 0;
  bool valid = false;
  bool rcSuffix = false;
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

  if (strncmp(cursor, "-rc", 3) == 0) parsed.rcSuffix = true;
  parsed.valid = true;
  return parsed;
}

}  // namespace

int semverCompareNewer(const char* currentVersion, const char* latestVersion) {
  const ParsedVersion current = parseVersion(currentVersion);
  const ParsedVersion latest = parseVersion(latestVersion);
  if (!current.valid || !latest.valid) return 0;

  if (latest.major != current.major) return latest.major > current.major ? 1 : -1;
  if (latest.minor != current.minor) return latest.minor > current.minor ? 1 : -1;
  if (latest.patch != current.patch) return latest.patch > current.patch ? 1 : -1;

  if (current.rcSuffix && !latest.rcSuffix) return 1;
  return 0;
}
