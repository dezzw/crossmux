#pragma once

#include <cstddef>

class SdCardFontRegistry {
 public:
  static const char* defaultWriteRoot() { return "/.fonts"; }

  bool findFamily(const char*) const { return false; }
};
