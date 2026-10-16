#include "HttpTransportFailure.h"

#include <HalClock.h>
#include <Logging.h>

#if defined(ESP_PLATFORM)
#include <WiFi.h>
#endif

namespace HttpTransportFailure {
namespace {

// wolfCrypt (error-crypt.h)
constexpr int kHashTypeE = -232;
constexpr int kAsnNoSignerE = -188;

// wolfSSL (error-ssl.h)
constexpr int kSocketErrorE = -308;
constexpr int kDomainNameMismatchE = -322;
constexpr int kVerifyCertErrorE = -329;
constexpr int kPeerClosedErrorE = -343;

#if defined(ESP_PLATFORM)
wl_status_t gWifiStatusAtClockGate = WL_NO_SHIELD;
#endif
bool gClockNotSetLatchedForWifiSession = false;

bool isDateError(const int err) { return err == -135 || err == -149 || err == -150 || err == -151 || err == -179; }

bool isMemoryError(const int err) { return err == -125; }

bool isConnectError(const int err) { return err == kSocketErrorE || err == kPeerClosedErrorE; }

bool isCertError(const int err) {
  return err == kSecureClientCaLoadError || err == kHashTypeE || err == -152 || err == -155 || err == -156 ||
         err == -157 || err == kAsnNoSignerE || err == -190 || err == -229 || err == -275 ||
         err == kDomainNameMismatchE || err == kVerifyCertErrorE;
}

void syncWifiSessionClockGate() {
#if defined(ESP_PLATFORM)
  const wl_status_t status = WiFi.status();
  if (status != gWifiStatusAtClockGate) {
    gWifiStatusAtClockGate = status;
    gClockNotSetLatchedForWifiSession = false;
  }
#endif
}

}  // namespace

HttpTransportTypes::TransportFailure fromWolfError(const int wolfErrorCode) {
  if (wolfErrorCode == 0) return HttpTransportTypes::TransportFailure::Connect;
  if (isMemoryError(wolfErrorCode)) return HttpTransportTypes::TransportFailure::TlsMemory;
  if (isDateError(wolfErrorCode)) return HttpTransportTypes::TransportFailure::TlsDate;
  if (isCertError(wolfErrorCode)) return HttpTransportTypes::TransportFailure::TlsCert;
  if (isConnectError(wolfErrorCode)) return HttpTransportTypes::TransportFailure::Connect;
  return HttpTransportTypes::TransportFailure::TlsHandshake;
}

void resetVerifiedTlsClockGate() {
  gClockNotSetLatchedForWifiSession = false;
#if defined(ESP_PLATFORM)
  gWifiStatusAtClockGate = WiFi.status();
#endif
}

bool ensureClockForVerifiedTls(HttpTransportTypes::TransportFailure& outFailure) {
  syncWifiSessionClockGate();
  if (halClock.hasValidTime()) return true;
  if (gClockNotSetLatchedForWifiSession) {
    outFailure = HttpTransportTypes::TransportFailure::ClockNotSet;
    return false;
  }
  LOG_INF("HTTP", "Clock invalid before verified TLS; attempting SNTP sync");
  if (halClock.syncNow() && halClock.hasValidTime()) return true;
  LOG_ERR("HTTP", "Verified TLS blocked: clock not set");
  gClockNotSetLatchedForWifiSession = true;
  outFailure = HttpTransportTypes::TransportFailure::ClockNotSet;
  return false;
}

}  // namespace HttpTransportFailure
