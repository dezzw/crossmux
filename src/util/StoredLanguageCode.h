#pragma once

#include <I18nKeys.h>

#include <cstring>

// True when settings.json still holds an obsolete language code (e.g. "FR")
// that parsed to English in this build and should be rewritten on save.
inline bool storedLanguageCodeNeedsResave(const char* storedCode, const Language parsed) {
  return parsed == Language::EN && std::strcmp(storedCode, "EN") != 0;
}
