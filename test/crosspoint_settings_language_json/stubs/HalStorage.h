#pragma once

#include <cstdint>
#include <string>

class HalFile {
 public:
  size_t write(const uint8_t*, size_t len) { return len; }
  int read(uint8_t*, size_t len) {
    (void)len;
    return 0;
  }
  void flush() {}
};

struct HalStorage {
  static HalStorage& getInstance() {
    static HalStorage instance;
    return instance;
  }
  static bool mkdir(const char*) { return true; }
  static bool exists(const char*) { return true; }
  static bool remove(const char*) { return true; }
  static bool rename(const char*, const char*) { return true; }
  static std::string readFile(const char*) { return {}; }
  static bool openFileForWrite(const char*, const char*, HalFile&) { return true; }
  static bool openFileForRead(const char*, const char*, HalFile&) { return false; }
};

#define Storage HalStorage::getInstance()
