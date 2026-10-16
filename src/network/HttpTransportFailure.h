#pragma once

#include "HttpTransportTypes.h"
#include "WolfSslTransportErrors.h"

namespace HttpTransportFailure {

constexpr int kSecureClientCaLoadError = WolfSslTransportErrors::kSecureClientCaLoadError;
static_assert(kSecureClientCaLoadError == WolfSslTransportErrors::kSecureClientCaLoadError);

// Maps a wolfSSL/wolfCrypt error from SecureClient to a transport category.
HttpTransportTypes::TransportFailure fromWolfError(int wolfErrorCode);

// Ensures UTC time is valid before verified TLS (one SNTP attempt per Wi-Fi session). Returns false and sets
// `outFailure` to ClockNotSet when the clock cannot be trusted.
bool ensureClockForVerifiedTls(HttpTransportTypes::TransportFailure& outFailure);

// Clears the per-Wi-Fi-session ClockNotSet latch (call when the link drops or reconnects).
void resetVerifiedTlsClockGate();

}  // namespace HttpTransportFailure
