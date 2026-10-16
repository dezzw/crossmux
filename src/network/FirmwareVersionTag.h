#pragma once

#include <cstddef>
#include <cstdint>

// Full-length CrossMux version marker embedded in every release image (unlike
// esp_app_desc_t.version, which is capped at 32 bytes and may not match
// CROSSPOINT_VERSION on stable builds).

namespace firmware_version_tag {

extern const char TAG[];

const char* versionString();
size_t versionStringLen();

class Scanner {
 public:
  void setExpected(const char* expected, size_t len);
  void feed(const uint8_t* data, size_t len);
  void markStreamComplete() { streamComplete = true; }
  bool mismatch() const { return mismatchFound; }
  bool matchConfirmed() const { return matchConfirmed_; }
  bool missingRequiredTag() const { return streamComplete && !matchConfirmed_ && !mismatchFound; }
  const char* foundVersion() const { return captured[0] != '\0' ? captured : ""; }

 private:
  const char* expected_ = nullptr;
  size_t expectedLen_ = 0;
  static constexpr size_t MAX_VERSION = 127;
  char captured[MAX_VERSION + 1] = {0};
  size_t valueLen = 0;
  size_t magicMatched = 0;
  bool capturing = false;
  bool mismatchFound = false;
  bool matchConfirmed_ = false;
  bool streamComplete = false;
};

}  // namespace firmware_version_tag
