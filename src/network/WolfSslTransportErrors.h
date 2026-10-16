#pragma once

// Single source for CrossMux ↔ SecureClient transport error sentinels (patched SDK uses the same value).
namespace WolfSslTransportErrors {
constexpr int kSecureClientCaLoadError = -2000;
}  // namespace WolfSslTransportErrors
