#pragma once

#include <cstddef>
#include <cstring>

class HalFile {
 public:
  size_t fileSize() const { return 0; }
};

class HalStorage {
 public:
  static HalStorage& getInstance() {
    static HalStorage instance;
    return instance;
  }

  bool openFileForRead(const char*, const char*, HalFile&) { return false; }
  void remove(const char*) {}
};

#define Storage HalStorage::getInstance()
