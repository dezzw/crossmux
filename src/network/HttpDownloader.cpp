#include "HttpDownloader.h"

#include <Arduino.h>
#include <HalSystem.h>
#include <Logging.h>
#include <Memory.h>
#include <Stream.h>
#include <base64.h>
#include <esp_crt_bundle.h>
#include <esp_http_client.h>
#include <esp_wifi.h>

#include <cstdio>
#include <functional>
#include <string>

#include "TlsCaBundle.h"

#if defined(FREEINK_NET_WOLFSSL)
#include <SecureHttpClient.h>

extern "C" void wolfSSL_Arduino_Serial_Print(const char* const msg) { LOG_DBG("WOLFSSL", "%s", msg); }
#endif

namespace {
#if !defined(FREEINK_NET_WOLFSSL)
constexpr int HTTP_RX_BUF = 2048;
constexpr int HTTP_TX_BUF = 512;
#endif
constexpr int HTTP_TIMEOUT_MS = 60000;
constexpr size_t READ_CHUNK = 1024;
constexpr int MAX_REDIRECTS = 5;
constexpr size_t DEVICE_MODEL_LENGTH_MAX = 32;
constexpr size_t USER_AGENT_CAPACITY = sizeof("CrossMux--" CROSSPOINT_VERSION) + DEVICE_MODEL_LENGTH_MAX;

HttpDownloader::TransportFailure gLastTransportFailure = HttpDownloader::TransportFailure::None;
int gLastHttpStatus = 0;

struct Sink {
  std::function<bool(const uint8_t*, size_t)> write;
  HttpDownloader::ProgressCallback progress;
  bool* cancelFlag = nullptr;
  bool verifyTlsPeer = false;
  size_t total = 0;
  size_t downloaded = 0;
};

void resetTransportDiagnostics() {
  gLastTransportFailure = HttpDownloader::TransportFailure::None;
  gLastHttpStatus = 0;
}

bool isRedirect(int status) {
  return status == 301 || status == 302 || status == 303 || status == 307 || status == 308;
}

bool rejectsCleartextRedirect(const std::string& secureStartUrl, const std::string& nextUrl) {
  if (secureStartUrl.rfind("https://", 0) == 0 && nextUrl.rfind("http://", 0) == 0) {
    LOG_ERR("HTTP", "Blocked HTTPS to HTTP redirect to %s", nextUrl.c_str());
    return true;
  }
  return false;
}

struct WifiPowerSaveGuard {
  WifiPowerSaveGuard() {
    esp_err_t err = esp_wifi_set_ps(WIFI_PS_NONE);
    if (err != ESP_OK) LOG_ERR("HTTP", "Failed to disable WiFi power-save: %d", err);
  }
  ~WifiPowerSaveGuard() {
    esp_err_t err = esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    if (err != ESP_OK) LOG_ERR("HTTP", "Failed to restore WiFi power-save: %d", err);
  }
};

#if defined(FREEINK_NET_WOLFSSL)
HttpDownloader::DownloadError runGetWolf(const std::string& startUrl, const std::string& username,
                                         const std::string& password, const char* userAgent, Sink& sink) {
  WifiPowerSaveGuard psGuard;
  const std::string secureStartUrl = startUrl;
  std::string url = startUrl;

  for (int hop = 0; hop <= MAX_REDIRECTS; ++hop) {
    freeink::SecureHttpClient http;
    http.setTimeout(HTTP_TIMEOUT_MS);
    if (sink.verifyTlsPeer) {
      http.setCACert(TlsCaBundle::pemRoots());
    } else {
      http.setInsecure();
    }
    if (!http.begin(url)) {
      LOG_ERR("HTTP", "wolfSSL bad URL: %s", url.c_str());
      gLastTransportFailure = HttpDownloader::TransportFailure::Connect;
      return HttpDownloader::HTTP_ERROR;
    }
    http.setUserAgent(userAgent);
    if (!username.empty() && !password.empty()) {
      const std::string credentials = username + ":" + password;
      const String encoded = base64::encode(credentials.c_str());
      http.addHeader("Authorization", std::string("Basic ") + encoded.c_str());
    }

    LOG_DBG("HTTP", "wolfSSL GET: %s", url.c_str());
    const int status = http.GET(
        [&http, &sink](const uint8_t* data, size_t len) {
          if (http.getStatus() != 200) return true;
          if (sink.total == 0 && http.hasContentLength()) sink.total = http.getContentLength();
          if (!sink.write(data, len)) return false;
          sink.downloaded += len;
          if (sink.progress && sink.total > 0) sink.progress(sink.downloaded, sink.total);
          return true;
        },
        [&sink]() { return sink.cancelFlag && *sink.cancelFlag; });

    if (http.aborted()) return HttpDownloader::ABORTED;
    if (status < 0) {
      LOG_ERR("HTTP", "wolfSSL request failed: status=%d url=%s", status, url.c_str());
      gLastTransportFailure = HttpDownloader::TransportFailure::Tls;
      return HttpDownloader::HTTP_ERROR;
    }
    gLastHttpStatus = status;
    if (isRedirect(status)) {
      const std::string location = http.getHeader("location");
      std::string nextUrl;
      if (location.empty() || !freeink::SecureHttpClient::resolveUrl(url, location, nextUrl)) {
        LOG_ERR("HTTP", "wolfSSL bad redirect: %d", status);
        gLastTransportFailure = HttpDownloader::TransportFailure::HttpStatus;
        return HttpDownloader::HTTP_ERROR;
      }
      if (rejectsCleartextRedirect(secureStartUrl, nextUrl)) {
        gLastTransportFailure = HttpDownloader::TransportFailure::HttpStatus;
        return HttpDownloader::HTTP_ERROR;
      }
      url = std::move(nextUrl);
      continue;
    }
    if (status != 200) {
      LOG_ERR("HTTP", "wolfSSL unexpected status: %d", status);
      gLastTransportFailure = HttpDownloader::TransportFailure::HttpStatus;
      return HttpDownloader::HTTP_ERROR;
    }
    if (http.callbackAborted()) {
      gLastTransportFailure = HttpDownloader::TransportFailure::FileIo;
      return HttpDownloader::FILE_ERROR;
    }
    if (!http.responseComplete()) {
      LOG_ERR("HTTP", "wolfSSL incomplete: got %zu of %zu bytes", sink.downloaded, sink.total);
      gLastTransportFailure = HttpDownloader::TransportFailure::Incomplete;
      return HttpDownloader::HTTP_ERROR;
    }
    return HttpDownloader::OK;
  }
  LOG_ERR("HTTP", "too many redirects");
  gLastTransportFailure = HttpDownloader::TransportFailure::HttpStatus;
  return HttpDownloader::HTTP_ERROR;
}
#endif

#if !defined(FREEINK_NET_WOLFSSL)
HttpDownloader::DownloadError runGet(const std::string& url, const std::string& username, const std::string& password,
                                     const char* userAgent, Sink& sink) {
  WifiPowerSaveGuard psGuard;
  esp_http_client_config_t config = {};
  config.url = url.c_str();
  config.buffer_size = HTTP_RX_BUF;
  config.buffer_size_tx = HTTP_TX_BUF;
  config.timeout_ms = HTTP_TIMEOUT_MS;
  if (sink.verifyTlsPeer) config.crt_bundle_attach = esp_crt_bundle_attach;
  config.keep_alive_enable = true;

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (!client) {
    LOG_ERR("HTTP", "client init failed");
    gLastTransportFailure = HttpDownloader::TransportFailure::Connect;
    return HttpDownloader::HTTP_ERROR;
  }

  esp_http_client_set_header(client, "User-Agent", userAgent);
  if (!username.empty() && !password.empty()) {
    const std::string credentials = username + ":" + password;
    const String header = "Basic " + base64::encode(credentials.c_str());
    esp_http_client_set_header(client, "Authorization", header.c_str());
  }

  esp_err_t err = esp_http_client_open(client, 0);
  if (err != ESP_OK) {
    LOG_ERR("HTTP", "open failed: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    gLastTransportFailure = HttpDownloader::TransportFailure::Connect;
    return HttpDownloader::HTTP_ERROR;
  }
  int64_t contentLength = esp_http_client_fetch_headers(client);
  int status = esp_http_client_get_status_code(client);
  for (int hop = 0; isRedirect(status) && hop < MAX_REDIRECTS; ++hop) {
    if (esp_http_client_set_redirection(client) != ESP_OK) break;
    esp_http_client_close(client);
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
      LOG_ERR("HTTP", "redirect open failed: %s", esp_err_to_name(err));
      esp_http_client_cleanup(client);
      gLastTransportFailure = HttpDownloader::TransportFailure::Connect;
      return HttpDownloader::HTTP_ERROR;
    }
    contentLength = esp_http_client_fetch_headers(client);
    status = esp_http_client_get_status_code(client);
  }
  gLastHttpStatus = status;
  if (status != 200) {
    LOG_ERR("HTTP", "unexpected status: %d", status);
    esp_http_client_cleanup(client);
    gLastTransportFailure = HttpDownloader::TransportFailure::HttpStatus;
    return HttpDownloader::HTTP_ERROR;
  }

  sink.total = contentLength > 0 ? static_cast<size_t>(contentLength) : 0;

  auto buf = makeUniqueNoThrow<char[]>(READ_CHUNK);
  if (!buf) {
    LOG_ERR("HTTP", "OOM: %u byte read buffer", (unsigned)READ_CHUNK);
    esp_http_client_cleanup(client);
    return HttpDownloader::HTTP_ERROR;
  }

  while (true) {
    if (sink.cancelFlag && *sink.cancelFlag) {
      esp_http_client_cleanup(client);
      return HttpDownloader::ABORTED;
    }
    const int read = esp_http_client_read(client, buf.get(), READ_CHUNK);
    if (read < 0) {
      LOG_ERR("HTTP", "read error after %zu bytes", sink.downloaded);
      esp_http_client_cleanup(client);
      gLastTransportFailure = HttpDownloader::TransportFailure::Incomplete;
      return HttpDownloader::HTTP_ERROR;
    }
    if (read == 0) break;
    if (!sink.write(reinterpret_cast<const uint8_t*>(buf.get()), read)) {
      esp_http_client_cleanup(client);
      gLastTransportFailure = HttpDownloader::TransportFailure::FileIo;
      return HttpDownloader::FILE_ERROR;
    }
    sink.downloaded += read;
    if (sink.progress && sink.total > 0) sink.progress(sink.downloaded, sink.total);
  }

  const bool complete = esp_http_client_is_complete_data_received(client);
  esp_http_client_cleanup(client);
  if (!complete) {
    LOG_ERR("HTTP", "incomplete: got %zu of %zu bytes", sink.downloaded, sink.total);
    gLastTransportFailure = HttpDownloader::TransportFailure::Incomplete;
    return HttpDownloader::HTTP_ERROR;
  }
  return HttpDownloader::OK;
}
#endif

HttpDownloader::DownloadError runGetSecure(const std::string& url, const std::string& username,
                                           const std::string& password, const Sink& sinkIn) {
  resetTransportDiagnostics();
  char userAgent[USER_AGENT_CAPACITY];
  const int userAgentLength =
      snprintf(userAgent, sizeof(userAgent), "CrossMux-%s-" CROSSPOINT_VERSION, HalSystem::getDeviceModel());
  if (userAgentLength < 0 || static_cast<size_t>(userAgentLength) >= sizeof(userAgent)) {
    LOG_ERR("HTTP", "User-Agent exceeds %zu bytes", sizeof(userAgent));
    return HttpDownloader::HTTP_ERROR;
  }
  Sink sink = sinkIn;
#if defined(FREEINK_NET_WOLFSSL)
  return runGetWolf(url, username, password, userAgent, sink);
#else
  return runGet(url, username, password, userAgent, sink);
#endif
}
}  // namespace

HttpDownloader::TransportFailure HttpDownloader::lastTransportFailure() { return gLastTransportFailure; }

int HttpDownloader::lastHttpStatusCode() { return gLastHttpStatus; }

bool HttpDownloader::fetchUrl(const std::string& url, Stream& outContent, const std::string& username,
                              const std::string& password) {
  return fetchUrl(url, outContent, FetchOptions{}, username, password);
}

bool HttpDownloader::fetchUrl(const std::string& url, Stream& outContent, const FetchOptions& options,
                              const std::string& username, const std::string& password) {
  LOG_DBG("HTTP", "Fetching: %s", url.c_str());
  Sink sink;
  sink.verifyTlsPeer = options.verifyTlsPeer;
  sink.write = [&outContent](const uint8_t* data, size_t len) { return outContent.write(data, len) == len; };
  return runGetSecure(url, username, password, sink) == OK;
}

bool HttpDownloader::fetchUrl(const std::string& url, std::string& outContent, const std::string& username,
                              const std::string& password) {
  return fetchUrl(url, outContent, FetchOptions{}, username, password);
}

bool HttpDownloader::fetchUrl(const std::string& url, std::string& outContent, const FetchOptions& options,
                              const std::string& username, const std::string& password) {
  LOG_DBG("HTTP", "Fetching: %s", url.c_str());
  outContent.clear();
  Sink sink;
  sink.verifyTlsPeer = options.verifyTlsPeer;
  sink.write = [&outContent](const uint8_t* data, size_t len) {
    outContent.append(reinterpret_cast<const char*>(data), len);
    return true;
  };
  return runGetSecure(url, username, password, sink) == OK;
}

bool HttpDownloader::fetchUrl(const std::string& url, const DataCallback& onData, const std::string& username,
                              const std::string& password) {
  return fetchUrl(url, onData, FetchOptions{}, username, password);
}

bool HttpDownloader::fetchUrl(const std::string& url, const DataCallback& onData, const FetchOptions& options,
                              const std::string& username, const std::string& password) {
  LOG_DBG("HTTP", "Fetching: %s", url.c_str());
  Sink sink;
  sink.verifyTlsPeer = options.verifyTlsPeer;
  sink.write = onData;
  return runGetSecure(url, username, password, sink) == OK;
}

HttpDownloader::DownloadError HttpDownloader::downloadToFile(const std::string& url, const std::string& destPath,
                                                             ProgressCallback progress, bool* cancelFlag,
                                                             const std::string& username, const std::string& password) {
  return downloadToFile(url, destPath, std::move(progress), cancelFlag, FetchOptions{}, username, password);
}

HttpDownloader::DownloadError HttpDownloader::downloadToFile(const std::string& url, const std::string& destPath,
                                                             ProgressCallback progress, bool* cancelFlag,
                                                             const FetchOptions& options, const std::string& username,
                                                             const std::string& password) {
  LOG_DBG("HTTP", "Downloading: %s -> %s", url.c_str(), destPath.c_str());

  if (Storage.exists(destPath.c_str())) {
    Storage.remove(destPath.c_str());
  }
  HalFile file;
  if (!Storage.openFileForWrite("HTTP", destPath.c_str(), file)) {
    LOG_ERR("HTTP", "Failed to open file for writing");
    gLastTransportFailure = TransportFailure::FileIo;
    return FILE_ERROR;
  }

  Sink sink;
  sink.verifyTlsPeer = options.verifyTlsPeer;
  sink.progress = std::move(progress);
  sink.cancelFlag = cancelFlag;
  sink.write = [&file](const uint8_t* data, size_t len) { return file.write(data, len) == len; };

  const DownloadError result = runGetSecure(url, username, password, sink);
  file.close();

  if (result != OK) {
    Storage.remove(destPath.c_str());
    return result;
  }
  if (sink.downloaded == 0) {
    LOG_ERR("HTTP", "no data received");
    Storage.remove(destPath.c_str());
    gLastTransportFailure = TransportFailure::Incomplete;
    return HTTP_ERROR;
  }
  LOG_DBG("HTTP", "Downloaded %zu bytes", sink.downloaded);
  return OK;
}
