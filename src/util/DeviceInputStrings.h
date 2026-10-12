#pragma once

#include <BoardConfig.h>
#include <I18n.h>

// Pick Waveshare 3.97 copy when that is the build target; other devices keep the default StrId.
inline const char* trInput(const StrId ws397, const StrId other) {
#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
  return I18N.get(ws397);
#else
  (void)ws397;
  return I18N.get(other);
#endif
}

inline StrId strIdInput(const StrId ws397, const StrId other) {
#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
  return ws397;
#else
  (void)ws397;
  return other;
#endif
}
