#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "ReleaseJsonParser.h"
#include "StreamingJsonParser.h"

class OtaManifestParser {
 public:
  using ReleaseNote = ReleaseJsonParser::ReleaseNote;
  static constexpr size_t RELEASE_NOTE_COUNT_MAX = ReleaseJsonParser::RELEASE_NOTE_COUNT_MAX;

  explicit OtaManifestParser(std::span<ReleaseNote> releaseNotes = {});

  OtaManifestParser(const OtaManifestParser&) = delete;
  OtaManifestParser& operator=(const OtaManifestParser&) = delete;

  void reset();
  void feed(const char* data, size_t len);

  bool foundVersion() const;
  bool foundBoardTag() const;
  bool foundFirmware() const;
  bool foundSignature() const;
  bool foundReleaseNotes() const;

  const char* getVersion() const;
  const char* getBoardTag() const;
  const char* getFirmwareUrl() const;
  const char* getFirmwareSha256() const;
  const char* getSignatureBase64() const;
  size_t getFirmwareSize() const;
  size_t getReleaseNoteCount() const;

 private:
  enum class Position : uint8_t {
    TOP_LEVEL,
    IN_ASSETS_ARRAY,
    IN_ASSET_OBJECT,
    IN_RELEASE_NOTES_ARRAY,
  };

  enum class LastKey : uint8_t {
    NONE,
    VERSION,
    BOARD_TAG,
    SIGNATURE,
    RELEASE_NOTES,
    ASSETS,
    ASSET_ROLE,
    ASSET_NAME,
    ASSET_URL,
    ASSET_SIZE,
    ASSET_SHA256,
    FIRMWARE_OBJECT,
    FIRMWARE_URL,
    FIRMWARE_SIZE,
    FIRMWARE_SHA256,
    FIRMWARE_SIGNATURE,
  };

  static void sOnKey(void* ctx, const char* key, size_t len);
  static void sOnString(void* ctx, const char* value, size_t len);
  static void sOnNumber(void* ctx, const char* value, size_t len);
  static void sOnBool(void* ctx, bool value);
  static void sOnNull(void* ctx);
  static void sOnObjectStart(void* ctx);
  static void sOnObjectEnd(void* ctx);
  static void sOnArrayStart(void* ctx);
  static void sOnArrayEnd(void* ctx);

  void commitAsset();
  void clearReleaseNotes();
  void commitReleaseNote(const char* value, size_t len);

  StreamingJsonParser parser;

  Position position;
  LastKey lastKey;
  uint8_t depth;
  uint8_t assetDepth;
  uint8_t firmwareObjectDepth;
  uint8_t releaseNotesDepth;

  char version[32];
  char boardTag[32];
  char firmwareUrl[512];
  char firmwareSha256[65];
  char signatureBase64[128];
  size_t firmwareSize;

  bool versionFound;
  bool boardTagFound;
  bool firmwareFound;
  bool signatureFound;
  bool releaseNotesFound;
  bool releaseNotesInvalid;

  size_t releaseNoteCount;
  std::span<ReleaseNote> releaseNotes;

  char currentAssetRole[16];
  char currentAssetUrl[512];
  char currentAssetSha256[65];
  size_t currentAssetSize;
};
