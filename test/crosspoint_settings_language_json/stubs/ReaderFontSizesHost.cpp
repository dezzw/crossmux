#include "ReaderFontSizes.h"

#include <cstddef>
#include <cstdint>

uint8_t snapToNearestPointSize(const uint8_t* sizes, const size_t count, const uint8_t pt) {
  if (count == 0 || sizes == nullptr) {
    return pt;
  }
  uint8_t best = sizes[0];
  for (size_t i = 1; i < count; ++i) {
    if (sizes[i] <= pt && sizes[i] >= best) {
      best = sizes[i];
    }
  }
  return best;
}
