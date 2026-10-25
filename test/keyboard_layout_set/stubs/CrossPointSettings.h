#pragma once

#include <cstdint>

struct CrossPointSettings {
  static CrossPointSettings& getInstance() {
    static CrossPointSettings instance;
    return instance;
  }
  uint16_t keyboardLayouts = 0;
};

#define SETTINGS CrossPointSettings::getInstance()
