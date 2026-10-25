#pragma once

#include <HalStorage.h>

namespace serialization {

template <typename T>
bool readPod(HalFile&, T&) {
  return false;
}

}  // namespace serialization
