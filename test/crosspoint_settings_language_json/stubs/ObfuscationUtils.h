#pragma once

#include <cstddef>
#include <string>

namespace obfuscation {

inline std::string deobfuscateFromBase64(const char*, size_t, bool* ok, bool* tooLong) {
  if (ok) *ok = false;
  if (tooLong) *tooLong = false;
  return {};
}

inline std::string obfuscateToBase64(const char* plain) { return plain ? plain : ""; }

}  // namespace obfuscation
