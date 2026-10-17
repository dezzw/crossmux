#include "HttpTransportFailureText.h"

#include <I18n.h>

const char* fontTransportFailureText(const HttpTransportTypes::TransportFailure failure) {
  switch (failure) {
    case HttpTransportTypes::TransportFailure::ClockNotSet:
      return tr(STR_FONT_ERR_CLOCK);
    case HttpTransportTypes::TransportFailure::TlsCert:
      return tr(STR_FONT_ERR_TLS_CERT);
    case HttpTransportTypes::TransportFailure::TlsDate:
      return tr(STR_FONT_ERR_TLS_DATE);
    case HttpTransportTypes::TransportFailure::TlsMemory:
      return tr(STR_FONT_ERR_TLS_MEMORY);
    case HttpTransportTypes::TransportFailure::TlsHandshake:
      return tr(STR_FONT_ERR_TLS_HANDSHAKE);
    case HttpTransportTypes::TransportFailure::Connect:
      return tr(STR_FONT_ERR_NETWORK);
    case HttpTransportTypes::TransportFailure::Incomplete:
      return tr(STR_FONT_ERR_HTTP);
    case HttpTransportTypes::TransportFailure::EmptyResponse:
      return tr(STR_FONT_ERR_EMPTY_RESPONSE);
    case HttpTransportTypes::TransportFailure::HttpStatus:
      return tr(STR_FONT_ERR_HTTP);
    case HttpTransportTypes::TransportFailure::FileIo:
      return tr(STR_FONT_ERR_SD);
    case HttpTransportTypes::TransportFailure::None:
      break;
  }
  return tr(STR_FONT_ERR_TLS);
}

const char* otaTransportFailureText(const HttpTransportTypes::TransportFailure failure) {
  switch (failure) {
    case HttpTransportTypes::TransportFailure::ClockNotSet:
      return tr(STR_OTA_ERR_CLOCK);
    case HttpTransportTypes::TransportFailure::TlsCert:
      return tr(STR_OTA_ERR_TLS_CERT);
    case HttpTransportTypes::TransportFailure::TlsDate:
      return tr(STR_OTA_ERR_TLS_DATE);
    case HttpTransportTypes::TransportFailure::TlsMemory:
      return tr(STR_OTA_ERR_TLS_MEMORY);
    case HttpTransportTypes::TransportFailure::TlsHandshake:
      return tr(STR_OTA_ERR_TLS_HANDSHAKE);
    case HttpTransportTypes::TransportFailure::Connect:
      return tr(STR_OTA_ERR_NETWORK);
    case HttpTransportTypes::TransportFailure::Incomplete:
      return tr(STR_OTA_ERR_HTTP);
    case HttpTransportTypes::TransportFailure::EmptyResponse:
      return tr(STR_OTA_ERR_EMPTY_RESPONSE);
    case HttpTransportTypes::TransportFailure::HttpStatus:
      return tr(STR_OTA_ERR_HTTP);
    case HttpTransportTypes::TransportFailure::FileIo:
      return tr(STR_OTA_ERR_STORAGE);
    case HttpTransportTypes::TransportFailure::None:
      break;
  }
  return tr(STR_OTA_ERR_TLS);
}
