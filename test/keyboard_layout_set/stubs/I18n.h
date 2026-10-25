#pragma once

#include "I18nKeys.h"

class I18n {
 public:
  static I18n& getInstance() {
    static I18n instance;
    return instance;
  }

  const char* get(const StrId id) const {
    (void)id;
    return "layout";
  }

  Language getLanguage() const { return _language; }
  void setLanguage(const Language lang) { _language = lang; }

 private:
  Language _language = Language::EN;
};

#define tr(id) I18n::getInstance().get(StrId::id)
#define I18N I18n::getInstance()
