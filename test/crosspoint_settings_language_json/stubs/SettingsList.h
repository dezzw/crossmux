#pragma once

#include <vector>

#include "SettingInfoHost.h"

inline bool isSettingAvailableOnBoard(const SettingInfo&) { return true; }

inline const std::vector<SettingInfo>& getBaseSettingsList() {
  static const std::vector<SettingInfo> kEmpty;
  return kEmpty;
}
