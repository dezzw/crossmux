#pragma once

#include "HttpTransportTypes.h"

namespace HttpTransportFailure {

// Sentinel returned by SecureClient when wolfSSL_CTX_load_verify_buffer fails.
constexpr int kSecureClientCaLoadError = -2000;

// Maps a wolfSSL/wolfCrypt error from SecureClient to a transport category.
HttpTransportTypes::TransportFailure fromWolfError(int wolfErrorCode);

// Ensures UTC time is valid before verified TLS (one SNTP attempt). Returns false and sets
// `outFailure` to ClockNotSet when the clock cannot be trusted.
bool ensureClockForVerifiedTls(HttpTransportTypes::TransportFailure& outFailure);

}  // namespace HttpTransportFailure
