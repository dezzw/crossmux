#include "HttpTransportFailure.h"

#include <HalClock.h>
#include <Logging.h>

namespace HttpTransportFailure {
namespace {

bool isDateError(const int err) { return err == -135 || err == -149 || err == -150 || err == -151 || err == -179; }

bool isMemoryError(const int err) { return err == -125 || err == -132; }

bool isCertError(const int err) {
  return err == kSecureClientCaLoadError || err == -152 || err == -155 || err == -156 || err == -157 || err == -188 ||
         err == -190 || err == -229 || err == -275;
}

}  // namespace

HttpTransportTypes::TransportFailure fromWolfError(const int wolfErrorCode) {
  if (wolfErrorCode == 0) return HttpTransportTypes::TransportFailure::Connect;
  if (isMemoryError(wolfErrorCode)) return HttpTransportTypes::TransportFailure::TlsMemory;
  if (isDateError(wolfErrorCode)) return HttpTransportTypes::TransportFailure::TlsDate;
  if (isCertError(wolfErrorCode)) return HttpTransportTypes::TransportFailure::TlsCert;
  return HttpTransportTypes::TransportFailure::TlsHandshake;
}

bool ensureClockForVerifiedTls(HttpTransportTypes::TransportFailure& outFailure) {
  if (halClock.hasValidTime()) return true;
  LOG_INF("HTTP", "Clock invalid before verified TLS; attempting SNTP sync");
  if (halClock.syncNow() && halClock.hasValidTime()) return true;
  LOG_ERR("HTTP", "Verified TLS blocked: clock not set");
  outFailure = HttpTransportTypes::TransportFailure::ClockNotSet;
  return false;
}

}  // namespace HttpTransportFailure
