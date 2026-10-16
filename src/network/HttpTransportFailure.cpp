#include "HttpTransportFailure.h"

#include <HalClock.h>
#include <Logging.h>

namespace HttpTransportFailure {
namespace {

// wolfCrypt (error-crypt.h)
constexpr int kMemoryE = -125;
constexpr int kDateE = -135;
constexpr int kAsnDateSzE = -149;
constexpr int kAsnBeforeDateE = -150;
constexpr int kAsnAfterDateE = -151;
constexpr int kAsnSigOidE = -152;
constexpr int kAsnSigConfirmE = -155;
constexpr int kAsnSigHashE = -156;
constexpr int kAsnSigKeyE = -157;
constexpr int kCrlCertDateErr = -179;
constexpr int kAsnNoSignerE = -188;
constexpr int kAsnCrlNoSignerE = -190;
constexpr int kSigVerifyE = -229;
constexpr int kHashTypeE = -232;
constexpr int kAsnSelfSignedE = -275;

// wolfSSL (error-ssl.h)
constexpr int kSocketErrorE = -308;
constexpr int kDomainNameMismatchE = -322;
constexpr int kVerifyCertErrorE = -329;
constexpr int kPeerClosedErrorE = -343;

bool gClockNotSetLatchedForWifiSession = false;

bool isDateError(const int err) {
  return err == kDateE || err == kAsnDateSzE || err == kAsnBeforeDateE || err == kAsnAfterDateE ||
         err == kCrlCertDateErr;
}

bool isMemoryError(const int err) { return err == kMemoryE; }

bool isConnectError(const int err) { return err == kSocketErrorE || err == kPeerClosedErrorE; }

bool isCertError(const int err) {
  return err == kSecureClientCaLoadError || err == kHashTypeE || err == kAsnSigOidE || err == kAsnSigConfirmE ||
         err == kAsnSigHashE || err == kAsnSigKeyE || err == kAsnNoSignerE || err == kAsnCrlNoSignerE ||
         err == kSigVerifyE || err == kAsnSelfSignedE || err == kDomainNameMismatchE || err == kVerifyCertErrorE;
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

void resetVerifiedTlsClockGate() { gClockNotSetLatchedForWifiSession = false; }

bool ensureClockForVerifiedTls(HttpTransportTypes::TransportFailure& outFailure) {
  if (halClock.hasValidTime()) {
    outFailure = HttpTransportTypes::TransportFailure::None;
    return true;
  }
  if (gClockNotSetLatchedForWifiSession) {
    outFailure = HttpTransportTypes::TransportFailure::ClockNotSet;
    return false;
  }
  LOG_INF("HTTP", "Clock invalid before verified TLS; attempting SNTP sync");
  if (halClock.syncNow() && halClock.hasValidTime()) {
    outFailure = HttpTransportTypes::TransportFailure::None;
    return true;
  }
  LOG_ERR("HTTP", "Verified TLS blocked: clock not set");
  gClockNotSetLatchedForWifiSession = true;
  outFailure = HttpTransportTypes::TransportFailure::ClockNotSet;
  return false;
}

}  // namespace HttpTransportFailure
