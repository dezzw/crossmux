#pragma once

#include <cstdint>

namespace HttpTransportTypes {

enum class TransportFailure : uint8_t {
  None = 0,
  Connect,
  ClockNotSet,
  TlsCert,
  TlsDate,
  TlsMemory,
  TlsHandshake,
  HttpStatus,
  Incomplete,
  FileIo,
};

}  // namespace HttpTransportTypes
