#pragma once

#include "HttpTransportTypes.h"
#include "WolfSslTransportErrors.h"

namespace HttpTransportFailure {

constexpr int kSecureClientCaLoadError = WolfSslTransportErrors::kSecureClientCaLoadError;

HttpTransportTypes::TransportFailure fromWolfError(int wolfErrorCode);

// Ensures UTC time is valid before verified TLS (one SNTP attempt until the Wi-Fi link resets). Returns false and sets
// `outFailure` to ClockNotSet when the clock cannot be trusted.
bool ensureClockForVerifiedTls(HttpTransportTypes::TransportFailure& outFailure);

// Clears the ClockNotSet latch (call when Wi-Fi connects, disconnects, or network activities exit).
void resetVerifiedTlsClockGate();

inline void onWifiConnected() { resetVerifiedTlsClockGate(); }
inline void onWifiDisconnected() { resetVerifiedTlsClockGate(); }

}  // namespace HttpTransportFailure
