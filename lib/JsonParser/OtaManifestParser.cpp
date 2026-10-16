#include "OtaManifestParser.h"

#include <cstdlib>
#include <cstring>

namespace {

constexpr char OTA_NOTES_FORBIDDEN[] = "`*_#<>[]{}\\";

void safeCopy(char* dst, size_t dstSize, const char* src, size_t srcLen) {
  const size_t n = srcLen < dstSize - 1 ? srcLen : dstSize - 1;
  memcpy(dst, src, n);
  dst[n] = '\0';
}

bool validReleaseNote(const char* value, const size_t len) {
  if (len == 0 || len >= ReleaseJsonParser::RELEASE_NOTE_SIZE || value[0] == ' ' || value[len - 1] == ' ') {
    return false;
  }
  for (size_t i = 0; i < len; ++i) {
    const auto c = static_cast<unsigned char>(value[i]);
    if (c < 0x20 || c == 0x7f || strchr(OTA_NOTES_FORBIDDEN, c) != nullptr) return false;
  }
  return true;
}

}  // namespace

OtaManifestParser::OtaManifestParser(const std::span<ReleaseNote> releaseNotes)
    : parser(JsonCallbacks{this, sOnKey, sOnString, sOnNumber, sOnBool, sOnNull, sOnObjectStart, sOnObjectEnd,
                           sOnArrayStart, sOnArrayEnd, nullptr}),
      releaseNotes(releaseNotes) {
  reset();
}

void OtaManifestParser::reset() {
  parser.reset();
  position = Position::TOP_LEVEL;
  lastKey = LastKey::NONE;
  depth = 0;
  assetDepth = 0;
  firmwareObjectDepth = 0;
  releaseNotesDepth = 0;
  version[0] = '\0';
  boardTag[0] = '\0';
  firmwareUrl[0] = '\0';
  firmwareSha256[0] = '\0';
  signatureBase64[0] = '\0';
  firmwareSize = 0;
  versionFound = false;
  boardTagFound = false;
  firmwareFound = false;
  signatureFound = false;
  releaseNotesFound = false;
  releaseNotesInvalid = false;
  releaseNoteCount = 0;
  clearReleaseNotes();
  currentAssetRole[0] = '\0';
  currentAssetUrl[0] = '\0';
  currentAssetSha256[0] = '\0';
  currentAssetSize = 0;
}

void OtaManifestParser::feed(const char* data, size_t len) { parser.feed(data, len); }

bool OtaManifestParser::foundVersion() const { return versionFound; }
bool OtaManifestParser::foundBoardTag() const { return boardTagFound; }
bool OtaManifestParser::foundFirmware() const { return firmwareFound; }
bool OtaManifestParser::foundSignature() const { return signatureFound; }
bool OtaManifestParser::foundReleaseNotes() const { return releaseNotesFound; }
const char* OtaManifestParser::getVersion() const { return version; }
const char* OtaManifestParser::getBoardTag() const { return boardTag; }
const char* OtaManifestParser::getFirmwareUrl() const { return firmwareUrl; }
const char* OtaManifestParser::getFirmwareSha256() const { return firmwareSha256; }
const char* OtaManifestParser::getSignatureBase64() const { return signatureBase64; }
size_t OtaManifestParser::getFirmwareSize() const { return firmwareSize; }
size_t OtaManifestParser::getReleaseNoteCount() const { return releaseNotesFound ? releaseNoteCount : 0; }

void OtaManifestParser::clearReleaseNotes() {
  releaseNotesFound = false;
  releaseNoteCount = 0;
  for (auto& note : releaseNotes) note[0] = '\0';
}

void OtaManifestParser::commitReleaseNote(const char* value, const size_t len) {
  if (releaseNotesInvalid) return;
  if (!validReleaseNote(value, len) || releaseNoteCount >= releaseNotes.size()) {
    releaseNotesInvalid = true;
    clearReleaseNotes();
    return;
  }
  safeCopy(releaseNotes[releaseNoteCount].data(), releaseNotes[releaseNoteCount].size(), value, len);
  ++releaseNoteCount;
}

void OtaManifestParser::commitAsset() {
  if (strcmp(currentAssetRole, "firmware") == 0) {
    if (currentAssetUrl[0] != '\0')
      safeCopy(firmwareUrl, sizeof(firmwareUrl), currentAssetUrl, strlen(currentAssetUrl));
    if (currentAssetSha256[0] != '\0') {
      safeCopy(firmwareSha256, sizeof(firmwareSha256), currentAssetSha256, strlen(currentAssetSha256));
    }
    if (currentAssetSize > 0) firmwareSize = currentAssetSize;
    firmwareFound = firmwareUrl[0] != '\0' && firmwareSha256[0] != '\0' && firmwareSize > 0;
  }
  currentAssetRole[0] = '\0';
  currentAssetUrl[0] = '\0';
  currentAssetSha256[0] = '\0';
  currentAssetSize = 0;
}

void OtaManifestParser::sOnKey(void* ctx, const char* key, size_t len) {
  auto* self = static_cast<OtaManifestParser*>(ctx);

  switch (self->position) {
    case Position::TOP_LEVEL:
      if (self->depth == 1) {
        if (len == 7 && memcmp(key, "version", 7) == 0)
          self->lastKey = LastKey::VERSION;
        else if (len == 8 && memcmp(key, "boardTag", 8) == 0)
          self->lastKey = LastKey::BOARD_TAG;
        else if (len == 9 && memcmp(key, "signature", 9) == 0)
          self->lastKey = LastKey::SIGNATURE;
        else if (len == 13 && memcmp(key, "release_notes", 13) == 0)
          self->lastKey = LastKey::RELEASE_NOTES;
        else if (len == 6 && memcmp(key, "assets", 6) == 0)
          self->lastKey = LastKey::ASSETS;
        else if (len == 8 && memcmp(key, "firmware", 8) == 0)
          self->lastKey = LastKey::FIRMWARE_OBJECT;
        else
          self->lastKey = LastKey::NONE;
      } else if (self->firmwareObjectDepth > 0) {
        if (len == 3 && memcmp(key, "url", 3) == 0)
          self->lastKey = LastKey::FIRMWARE_URL;
        else if (len == 4 && memcmp(key, "size", 4) == 0)
          self->lastKey = LastKey::FIRMWARE_SIZE;
        else if (len == 6 && memcmp(key, "sha256", 6) == 0)
          self->lastKey = LastKey::FIRMWARE_SHA256;
        else if (len == 9 && memcmp(key, "signature", 9) == 0)
          self->lastKey = LastKey::FIRMWARE_SIGNATURE;
        else
          self->lastKey = LastKey::NONE;
      }
      break;
    case Position::IN_RELEASE_NOTES_ARRAY:
      self->releaseNotesInvalid = true;
      self->lastKey = LastKey::NONE;
      break;
    case Position::IN_ASSET_OBJECT:
      if (self->assetDepth == 1) {
        if (len == 4 && memcmp(key, "role", 4) == 0)
          self->lastKey = LastKey::ASSET_ROLE;
        else if (len == 4 && memcmp(key, "name", 4) == 0)
          self->lastKey = LastKey::ASSET_NAME;
        else if (len == 3 && memcmp(key, "url", 3) == 0)
          self->lastKey = LastKey::ASSET_URL;
        else if (len == 4 && memcmp(key, "size", 4) == 0)
          self->lastKey = LastKey::ASSET_SIZE;
        else if (len == 6 && memcmp(key, "sha256", 6) == 0)
          self->lastKey = LastKey::ASSET_SHA256;
        else
          self->lastKey = LastKey::NONE;
      }
      break;
    case Position::IN_ASSETS_ARRAY:
      break;
  }
}

void OtaManifestParser::sOnString(void* ctx, const char* value, size_t len) {
  auto* self = static_cast<OtaManifestParser*>(ctx);

  if (self->position == Position::IN_RELEASE_NOTES_ARRAY) {
    if (self->releaseNotesDepth == 0)
      self->commitReleaseNote(value, len);
    else
      self->releaseNotesInvalid = true;
    self->lastKey = LastKey::NONE;
    return;
  }

  switch (self->lastKey) {
    case LastKey::VERSION:
      if (self->position == Position::TOP_LEVEL && self->depth == 1) {
        safeCopy(self->version, sizeof(self->version), value, len);
        self->versionFound = true;
      }
      break;
    case LastKey::BOARD_TAG:
      if (self->position == Position::TOP_LEVEL && self->depth == 1) {
        safeCopy(self->boardTag, sizeof(self->boardTag), value, len);
        self->boardTagFound = true;
      }
      break;
    case LastKey::SIGNATURE:
      if (self->position == Position::TOP_LEVEL && self->depth == 1) {
        safeCopy(self->signatureBase64, sizeof(self->signatureBase64), value, len);
        self->signatureFound = true;
      }
      break;
    case LastKey::FIRMWARE_URL:
      if (self->firmwareObjectDepth > 0) {
        safeCopy(self->firmwareUrl, sizeof(self->firmwareUrl), value, len);
      } else if (self->position == Position::IN_ASSET_OBJECT && self->assetDepth == 1) {
        safeCopy(self->currentAssetUrl, sizeof(self->currentAssetUrl), value, len);
      }
      break;
    case LastKey::FIRMWARE_SHA256:
      if (self->firmwareObjectDepth > 0) {
        safeCopy(self->firmwareSha256, sizeof(self->firmwareSha256), value, len);
      } else if (self->position == Position::IN_ASSET_OBJECT && self->assetDepth == 1) {
        safeCopy(self->currentAssetSha256, sizeof(self->currentAssetSha256), value, len);
      }
      break;
    case LastKey::FIRMWARE_SIGNATURE:
      if (self->firmwareObjectDepth > 0) {
        safeCopy(self->signatureBase64, sizeof(self->signatureBase64), value, len);
        self->signatureFound = true;
      }
      break;
    case LastKey::ASSET_ROLE:
      if (self->position == Position::IN_ASSET_OBJECT && self->assetDepth == 1)
        safeCopy(self->currentAssetRole, sizeof(self->currentAssetRole), value, len);
      break;
    case LastKey::ASSET_NAME:
      if (self->position == Position::IN_ASSET_OBJECT && self->assetDepth == 1 && len == 12 &&
          memcmp(value, "firmware.bin", 12) == 0) {
        safeCopy(self->currentAssetRole, sizeof(self->currentAssetRole), "firmware", 8);
      }
      break;
    case LastKey::ASSET_URL:
      if (self->position == Position::IN_ASSET_OBJECT && self->assetDepth == 1)
        safeCopy(self->currentAssetUrl, sizeof(self->currentAssetUrl), value, len);
      break;
    case LastKey::ASSET_SHA256:
      if (self->position == Position::IN_ASSET_OBJECT && self->assetDepth == 1)
        safeCopy(self->currentAssetSha256, sizeof(self->currentAssetSha256), value, len);
      break;
    case LastKey::FIRMWARE_OBJECT:
    case LastKey::NONE:
    case LastKey::RELEASE_NOTES:
    case LastKey::ASSETS:
    case LastKey::ASSET_SIZE:
    case LastKey::FIRMWARE_SIZE:
      break;
  }
  self->lastKey = LastKey::NONE;
}

void OtaManifestParser::sOnNumber(void* ctx, const char* value, size_t /*len*/) {
  auto* self = static_cast<OtaManifestParser*>(ctx);
  if (self->position == Position::IN_RELEASE_NOTES_ARRAY) self->releaseNotesInvalid = true;

  if (self->lastKey == LastKey::ASSET_SIZE && self->position == Position::IN_ASSET_OBJECT && self->assetDepth == 1) {
    self->currentAssetSize = static_cast<size_t>(strtoul(value, nullptr, 10));
  } else if (self->lastKey == LastKey::FIRMWARE_SIZE && self->firmwareObjectDepth > 0) {
    self->firmwareSize = static_cast<size_t>(strtoul(value, nullptr, 10));
  }
  self->lastKey = LastKey::NONE;
}

void OtaManifestParser::sOnBool(void* ctx, bool /*value*/) {
  auto* self = static_cast<OtaManifestParser*>(ctx);
  if (self->position == Position::IN_RELEASE_NOTES_ARRAY) self->releaseNotesInvalid = true;
  self->lastKey = LastKey::NONE;
}

void OtaManifestParser::sOnNull(void* ctx) {
  auto* self = static_cast<OtaManifestParser*>(ctx);
  if (self->position == Position::IN_RELEASE_NOTES_ARRAY) self->releaseNotesInvalid = true;
  self->lastKey = LastKey::NONE;
}

void OtaManifestParser::sOnObjectStart(void* ctx) {
  auto* self = static_cast<OtaManifestParser*>(ctx);

  switch (self->position) {
    case Position::TOP_LEVEL:
      self->depth++;
      if (self->lastKey == LastKey::FIRMWARE_OBJECT) self->firmwareObjectDepth++;
      self->lastKey = LastKey::NONE;
      break;
    case Position::IN_ASSETS_ARRAY:
      self->position = Position::IN_ASSET_OBJECT;
      self->assetDepth = 1;
      self->currentAssetRole[0] = '\0';
      self->currentAssetUrl[0] = '\0';
      self->currentAssetSha256[0] = '\0';
      self->currentAssetSize = 0;
      self->lastKey = LastKey::NONE;
      break;
    case Position::IN_ASSET_OBJECT:
      self->assetDepth++;
      self->lastKey = LastKey::NONE;
      break;
    case Position::IN_RELEASE_NOTES_ARRAY:
      self->releaseNotesInvalid = true;
      self->releaseNotesDepth++;
      self->lastKey = LastKey::NONE;
      break;
  }
}

void OtaManifestParser::sOnObjectEnd(void* ctx) {
  auto* self = static_cast<OtaManifestParser*>(ctx);

  switch (self->position) {
    case Position::TOP_LEVEL:
      if (self->firmwareObjectDepth > 0) {
        self->firmwareObjectDepth--;
        if (self->firmwareObjectDepth == 0 && self->firmwareUrl[0] != '\0' && self->firmwareSha256[0] != '\0' &&
            self->firmwareSize > 0) {
          self->firmwareFound = true;
        }
      } else if (self->depth > 0) {
        self->depth--;
      }
      break;
    case Position::IN_ASSET_OBJECT:
      self->assetDepth--;
      if (self->assetDepth == 0) {
        self->commitAsset();
        self->position = Position::IN_ASSETS_ARRAY;
      }
      self->lastKey = LastKey::NONE;
      break;
    case Position::IN_RELEASE_NOTES_ARRAY:
      if (self->releaseNotesDepth > 0) self->releaseNotesDepth--;
      self->lastKey = LastKey::NONE;
      break;
    default:
      break;
  }
}

void OtaManifestParser::sOnArrayStart(void* ctx) {
  auto* self = static_cast<OtaManifestParser*>(ctx);

  switch (self->position) {
    case Position::TOP_LEVEL:
      if (self->lastKey == LastKey::RELEASE_NOTES && self->depth == 1) {
        self->clearReleaseNotes();
        self->releaseNotesInvalid = self->releaseNotes.empty();
        self->releaseNotesDepth = 0;
        self->position = Position::IN_RELEASE_NOTES_ARRAY;
      } else if (self->lastKey == LastKey::ASSETS && self->depth == 1) {
        self->position = Position::IN_ASSETS_ARRAY;
      } else {
        self->depth++;
      }
      self->lastKey = LastKey::NONE;
      break;
    case Position::IN_ASSET_OBJECT:
      self->assetDepth++;
      self->lastKey = LastKey::NONE;
      break;
    case Position::IN_RELEASE_NOTES_ARRAY:
      self->releaseNotesInvalid = true;
      self->releaseNotesDepth++;
      self->lastKey = LastKey::NONE;
      break;
    default:
      break;
  }
}

void OtaManifestParser::sOnArrayEnd(void* ctx) {
  auto* self = static_cast<OtaManifestParser*>(ctx);

  switch (self->position) {
    case Position::TOP_LEVEL:
      if (self->depth > 0) self->depth--;
      break;
    case Position::IN_ASSETS_ARRAY:
      self->position = Position::TOP_LEVEL;
      break;
    case Position::IN_RELEASE_NOTES_ARRAY:
      if (self->releaseNotesDepth > 0) {
        self->releaseNotesDepth--;
      } else {
        if (!self->releaseNotesInvalid && self->releaseNoteCount >= 2)
          self->releaseNotesFound = true;
        else
          self->clearReleaseNotes();
        self->position = Position::TOP_LEVEL;
      }
      self->lastKey = LastKey::NONE;
      break;
    case Position::IN_ASSET_OBJECT:
      self->assetDepth--;
      self->lastKey = LastKey::NONE;
      break;
  }
}
