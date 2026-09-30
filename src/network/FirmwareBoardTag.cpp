#include "FirmwareBoardTag.h"

#include <BoardConfig.h>

#include <cstring>

#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
#define CROSSPOINT_BOARD_NAME "waveshare_epaper_397"
#elif CROSSPOINT_EMULATED
#define CROSSPOINT_BOARD_NAME "simulator"
#else
#error "FirmwareBoardTag: Waveshare-only fork expects FREEINK_DEVICE_WAVESHARE_EPAPER_397 or CROSSPOINT_EMULATED"
#endif

namespace board_tag {

namespace {
constexpr size_t MAGIC_LEN = sizeof("CROSSPOINT-BOARD-V1:") - 1;
}  // namespace

const char TAG[] = "CROSSPOINT-BOARD-V1:" CROSSPOINT_BOARD_NAME ";";

const char* boardName() { return TAG + MAGIC_LEN; }
size_t boardNameLen() { return sizeof(TAG) - 1 - MAGIC_LEN - 1; }

void Scanner::feed(const uint8_t* data, size_t len) {
  if (mismatchFound) return;
  for (size_t i = 0; i < len; i++) {
    const char c = static_cast<char>(data[i]);
    if (capturing) {
      if (c == ';') {
        capturing = false;
        captured[nameLen] = '\0';
        if (nameLen != boardNameLen() || memcmp(captured, boardName(), nameLen) != 0) {
          mismatchFound = true;
          return;
        }
      } else if (nameLen < MAX_NAME && c > 0x20 && c < 0x7F) {
        captured[nameLen++] = c;
      } else {
        capturing = false;
      }
      continue;
    }
    if (c == TAG[magicMatched]) {
      if (++magicMatched == MAGIC_LEN) {
        magicMatched = 0;
        capturing = true;
        nameLen = 0;
      }
    } else {
      magicMatched = (c == TAG[0]) ? 1 : 0;
    }
  }
}

}  // namespace board_tag
