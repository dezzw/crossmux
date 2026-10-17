#pragma once

#include <cstdint>
#include <string>

using byte = uint8_t;

class String : public std::string {
 public:
  String() = default;
  String(const char* s) : std::string(s ? s : "") {}
};
