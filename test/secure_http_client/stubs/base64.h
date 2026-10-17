#pragma once

#include <string>

namespace base64 {
inline std::string encode(const char* input) { return std::string(input); }
}  // namespace base64
