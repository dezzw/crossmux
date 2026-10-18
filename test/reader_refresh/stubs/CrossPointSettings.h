#pragma once

class CrossPointSettings {
 public:
  static CrossPointSettings& getInstance() {
    static CrossPointSettings instance;
    return instance;
  }

  int refreshFrequency = 15;
  int getRefreshFrequency() const { return refreshFrequency; }
};

#define SETTINGS CrossPointSettings::getInstance()
