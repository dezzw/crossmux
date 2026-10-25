#include <ArduinoJson.h>

#include <string>

#include "PersistableStore.h"

namespace {

std::string g_settingsJson;

}  // namespace

int g_crosspoint_test_write_count = 0;

void crosspoint_test_set_settings_json(const char* json) { g_settingsJson = json ? json : ""; }

bool PersistableStoreBase::writeDocToFile(const char* path, const JsonDocument& doc) {
  (void)path;
  (void)doc;
  ++g_crosspoint_test_write_count;
  return true;
}

bool PersistableStoreBase::readDocFromFile(const char* path, JsonDocument& doc) {
  (void)path;
  if (g_settingsJson.empty()) {
    return false;
  }
  const DeserializationError error = deserializeJson(doc, g_settingsJson);
  return !error;
}

std::string PersistableStoreBase::extractPassword(JsonVariantConst doc, bool& needsResave) {
  (void)doc;
  needsResave = false;
  return {};
}

std::string PersistableStoreBase::extractPassword(JsonVariantConst doc, bool& needsResave, const size_t maxLength,
                                                  bool& valid) {
  (void)doc;
  (void)maxLength;
  needsResave = false;
  valid = false;
  return {};
}
