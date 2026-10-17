#pragma once

#include <memory>

template <typename T>
std::unique_ptr<T[]> makeUniqueNoThrow(size_t count) {
  return std::unique_ptr<T[]>(new (std::nothrow) T[count]);
}
