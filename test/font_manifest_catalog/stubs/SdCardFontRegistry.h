#pragma once

#include <cstddef>

class SdCardFontRegistry {
 public:
  static const char* defaultWriteRoot() { return "/.fonts"; }

  const void* findFamily(const char*) const { return nullptr; }

  void discover() {}
};
