#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "CrossPointSettings.h"
#include "I18nKeys.h"

enum class SettingType { TOGGLE, ENUM, ACTION, VALUE, STRING };

struct SettingInfo {
  StrId nameId{};
  SettingType type{};
  uint8_t CrossPointSettings::* valuePtr = nullptr;
  int8_t CrossPointSettings::* signedValuePtr = nullptr;
  std::vector<StrId> enumValues;
  std::vector<std::string> enumStringValues;
  const char* key = nullptr;
  StrId category = StrId::STR_NONE_OPT;
  bool obfuscated = false;
  bool inTextSettings = false;
  bool inReadingStatsSettings = false;
  bool managedEnumPicker = false;
  struct ValueRange {
    int16_t min;
    int16_t max;
    int16_t step;
  };
  ValueRange valueRange = {};
  size_t stringOffset = 0;
  size_t stringMaxLen = 0;
  std::function<uint8_t()> valueGetter;
  std::function<void(uint8_t)> valueSetter;
  std::function<std::string()> stringGetter;
  std::function<void(const std::string&)> stringSetter;
};
