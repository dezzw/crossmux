#include "FirmwareVersionTag.h"

#include <cstring>

#ifndef CROSSPOINT_VERSION
#error "CROSSPOINT_VERSION must be defined for FirmwareVersionTag"
#endif

namespace firmware_version_tag {

namespace {
constexpr size_t MAGIC_LEN = sizeof("CROSSPOINT-VERSION-V1:") - 1;
}  // namespace

const char TAG[] = "CROSSPOINT-VERSION-V1:" CROSSPOINT_VERSION ";";

const char* versionString() { return TAG + MAGIC_LEN; }

size_t versionStringLen() {
  const size_t total = sizeof(TAG) - 1;
  return total - MAGIC_LEN - 1;
}

void Scanner::setExpected(const char* expected, size_t len) {
  expected_ = expected;
  expectedLen_ = len;
}

void Scanner::feed(const uint8_t* data, size_t len) {
  if (mismatchFound) return;
  for (size_t i = 0; i < len; i++) {
    const char c = static_cast<char>(data[i]);
    if (capturing) {
      if (c == ';') {
        capturing = false;
        captured[valueLen] = '\0';
        if (expected_ == nullptr || expectedLen_ == 0 || valueLen != expectedLen_ ||
            memcmp(captured, expected_, valueLen) != 0) {
          mismatchFound = true;
          return;
        }
        matchConfirmed_ = true;
      } else if (valueLen < MAX_VERSION && c > 0x20 && c < 0x7F) {
        captured[valueLen++] = c;
      } else {
        capturing = false;
      }
      continue;
    }
    if (c == TAG[magicMatched]) {
      if (++magicMatched == MAGIC_LEN) {
        magicMatched = 0;
        capturing = true;
        valueLen = 0;
      }
    } else {
      magicMatched = (c == TAG[0]) ? 1 : 0;
    }
  }
}

}  // namespace firmware_version_tag
