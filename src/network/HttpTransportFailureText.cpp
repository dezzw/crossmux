#include "HttpTransportFailureText.h"

#include <I18n.h>

const char* fontTransportFailureText(const HttpDownloader::TransportFailure failure) {
  switch (failure) {
    case HttpDownloader::TransportFailure::ClockNotSet:
      return tr(STR_FONT_ERR_CLOCK);
    case HttpDownloader::TransportFailure::TlsCert:
      return tr(STR_FONT_ERR_TLS_CERT);
    case HttpDownloader::TransportFailure::TlsDate:
      return tr(STR_FONT_ERR_TLS_DATE);
    case HttpDownloader::TransportFailure::TlsMemory:
      return tr(STR_FONT_ERR_TLS_MEMORY);
    case HttpDownloader::TransportFailure::TlsHandshake:
      return tr(STR_FONT_ERR_TLS_HANDSHAKE);
    case HttpDownloader::TransportFailure::Connect:
    case HttpDownloader::TransportFailure::Incomplete:
      return tr(STR_FONT_ERR_NETWORK);
    case HttpDownloader::TransportFailure::HttpStatus:
      return tr(STR_FONT_ERR_HTTP);
    case HttpDownloader::TransportFailure::FileIo:
      return tr(STR_FONT_ERR_SD);
    case HttpDownloader::TransportFailure::None:
      break;
  }
  return tr(STR_FONT_ERR_TLS);
}

const char* otaTransportFailureText(const HttpDownloader::TransportFailure failure) {
  switch (failure) {
    case HttpDownloader::TransportFailure::ClockNotSet:
      return tr(STR_OTA_ERR_CLOCK);
    case HttpDownloader::TransportFailure::TlsCert:
      return tr(STR_OTA_ERR_TLS_CERT);
    case HttpDownloader::TransportFailure::TlsDate:
      return tr(STR_OTA_ERR_TLS_DATE);
    case HttpDownloader::TransportFailure::TlsMemory:
      return tr(STR_OTA_ERR_TLS_MEMORY);
    case HttpDownloader::TransportFailure::TlsHandshake:
      return tr(STR_OTA_ERR_TLS_HANDSHAKE);
    case HttpDownloader::TransportFailure::Connect:
    case HttpDownloader::TransportFailure::Incomplete:
      return tr(STR_OTA_ERR_NETWORK);
    case HttpDownloader::TransportFailure::HttpStatus:
      return tr(STR_OTA_ERR_HTTP);
    case HttpDownloader::TransportFailure::FileIo:
      return tr(STR_OTA_ERR_STORAGE);
    case HttpDownloader::TransportFailure::None:
      break;
  }
  return tr(STR_OTA_ERR_TLS);
}
