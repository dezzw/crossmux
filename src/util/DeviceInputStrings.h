#pragma once

#include <BoardConfig.h>
#include <I18n.h>

inline const char* settingDisplayName(const StrId id) {
#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
  if (id == StrId::STR_BACK_SHORT_TO_FILE_BROWSER) {
    return I18N.get(StrId::STR_BACK_SHORT_TO_FILE_BROWSER_WS397);
  }
#endif
  return I18N.get(id);
}
