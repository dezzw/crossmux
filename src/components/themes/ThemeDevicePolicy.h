#pragma once

#ifndef FREEINK_DEVICE_WAVESHARE_EPAPER_397
#define FREEINK_DEVICE_WAVESHARE_EPAPER_397 0
#endif

// Device-specific UI visibility and metric adjustments shared by BaseTheme and UITheme.
inline bool deviceKeyInstructionTipsVisible() {
#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
  return false;
#else
  return true;
#endif
}

inline bool deviceButtonHintsVisible(const bool settingsShowButtonHints) {
#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
  (void)settingsShowButtonHints;
  return false;
#else
  return settingsShowButtonHints;
#endif
}

inline void deviceApplyButtonHintMetrics(int& buttonHintsHeight, int& sideButtonHintsWidth, const bool showButtonHints) {
  if (!showButtonHints) {
    buttonHintsHeight = 0;
    sideButtonHintsWidth = 0;
  }
}
