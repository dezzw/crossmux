#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>

class HalFile {
 public:
  std::FILE* fp = nullptr;

  size_t write(const uint8_t* data, size_t len) {
    if (!fp) return 0;
    return std::fwrite(data, 1, len, fp);
  }

  void close() {
    if (fp) {
      std::fclose(fp);
      fp = nullptr;
    }
  }
};

class HalStorage {
 public:
  static HalStorage& getInstance() {
    static HalStorage instance;
    return instance;
  }

  std::unordered_map<std::string, bool> existsMap;

  bool exists(const char* path) const { return existsMap.find(path) != existsMap.end(); }

  void remove(const char* path) {
    existsMap.erase(path);
    std::remove(path);
  }

  bool openFileForWrite(const char*, const char* path, HalFile& file) {
    file.fp = std::fopen(path, "wb");
    if (!file.fp) return false;
    existsMap[path] = true;
    return true;
  }
};

#define Storage HalStorage::getInstance()
