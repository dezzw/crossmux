#include "OtaUpdater.h"

// clang-format off
#include "HttpDownloader.h"
#include <HalSystem.h>
#include <Logging.h>
#include <OtaManifestParser.h>
#include <OtaSigningPayload.h>
#include <OtaSignatureVerify.h>
#include <SemVerCompare.h>
#include <esp_app_format.h>
#include <esp_ota_ops.h>
#include <mbedtls/base64.h>
#include <esp_wifi.h>
#include <mbedtls/sha256.h>
// clang-format on

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

#include "CrossMuxEndpoints.h"
#include "FirmwareBoardTag.h"
#include "FirmwareFlasher.h"
#include "OtaSigningPublicKey.h"

#if defined(FREEINK_DEVICE_WAVESHARE_EPAPER_397) && !defined(SIMULATOR) && !defined(CROSSPOINT_HOST_TEST)
#include <sdkconfig.h>
#if !defined(CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE) || !CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
// cppcheck-suppress preprocessorErrorDirective
#error "Waveshare OTA requires CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y (prebuilt dio_opi sdkconfig)"
#endif
#endif

namespace {

constexpr size_t kManifestUrlCapacity = 192;
constexpr size_t kSha256HexLen = 64;
constexpr HttpDownloader::FetchOptions kVerifiedTls{.verifyTlsPeer = true};

bool hexEquals(const char* expectedHex, const uint8_t digest[32]) {
  if (expectedHex == nullptr || strlen(expectedHex) != kSha256HexLen) return false;
  char actual[kSha256HexLen + 1];
  for (size_t i = 0; i < 32; ++i) {
    snprintf(actual + i * 2, 3, "%02x", digest[i]);
  }
  for (size_t i = 0; i < kSha256HexLen; ++i) {
    if (std::tolower(static_cast<unsigned char>(actual[i])) !=
        std::tolower(static_cast<unsigned char>(expectedHex[i]))) {
      return false;
    }
  }
  return true;
}

bool decodeBase64Signature(const std::string& encoded, uint8_t* out, size_t outCapacity, size_t& outLen) {
  outLen = 0;
  if (encoded.empty() || outCapacity == 0) return false;
  size_t decodedLen = 0;
  const int rc = mbedtls_base64_decode(out, outCapacity, &decodedLen,
                                       reinterpret_cast<const unsigned char*>(encoded.data()), encoded.size());
  if (rc != 0 || decodedLen == 0) return false;
  outLen = decodedLen;
  return true;
}

}  // namespace

OtaUpdater::OtaUpdaterError OtaUpdater::checkForUpdate(const Channel requestedChannel) {
  channel = requestedChannel;
  char manifestUrl[kManifestUrlCapacity];
  const int manifestUrlLength = channel == Channel::Stable
                                    ? CrossMuxEndpoints::formatOtaStableManifestUrl(manifestUrl, sizeof(manifestUrl))
                                    : CrossMuxEndpoints::formatOtaNightlyManifestUrl(manifestUrl, sizeof(manifestUrl));
  if (manifestUrlLength < 0 || static_cast<size_t>(manifestUrlLength) >= sizeof(manifestUrl)) {
    LOG_ERR("OTA", "Manifest URL exceeds %zu bytes", sizeof(manifestUrl));
    return INTERNAL_UPDATE_ERROR;
  }
  LOG_DBG("OTA", "Checking %s channel (current: %s)", channel == Channel::Nightly ? "nightly" : "stable",
          CROSSPOINT_VERSION);

  OtaManifestParser manifestParser(releaseNotes);
  const bool ok = HttpDownloader::fetchUrl(
      manifestUrl,
      [&manifestParser](const uint8_t* data, size_t len) {
        manifestParser.feed(reinterpret_cast<const char*>(data), len);
        return true;
      },
      kVerifiedTls);
  if (!ok) {
    LOG_ERR("OTA", "Manifest fetch failed");
    return HTTP_ERROR;
  }

  if (!manifestParser.foundVersion() || !manifestParser.foundBoardTag() || !manifestParser.foundFirmware() ||
      !manifestParser.foundSignature()) {
    LOG_ERR("OTA", "Manifest missing required fields");
    return JSON_PARSE_ERROR;
  }

  if (strcmp(manifestParser.getBoardTag(), board_tag::boardName()) != 0) {
    LOG_ERR("OTA", "Manifest boardTag mismatch: %s", manifestParser.getBoardTag());
    return WRONG_DEVICE_ERROR;
  }

  latestVersion = manifestParser.getVersion();
  otaBoardTag = manifestParser.getBoardTag();
  otaUrl = manifestParser.getFirmwareUrl();
  otaSha256Hex = manifestParser.getFirmwareSha256();
  otaSignatureBase64 = manifestParser.getSignatureBase64();
  otaSize = manifestParser.getFirmwareSize();
  totalSize = otaSize;
  updateAvailable = true;
  releaseNoteCount = manifestParser.getReleaseNoteCount();

  LOG_DBG("OTA", "Found update: version=%s size=%zu notes=%zu", latestVersion.c_str(), otaSize, releaseNoteCount);
  LOG_DBG("OTA", "Firmware URL: %s", otaUrl.c_str());
  return OK;
}

bool OtaUpdater::isUpdateNewer() const {
  if (!updateAvailable || latestVersion.empty()) return false;
  return semverCompareNewer(CROSSPOINT_VERSION, latestVersion.c_str()) > 0;
}

const std::string& OtaUpdater::getLatestVersion() const { return latestVersion; }

OtaUpdater::OtaUpdaterError OtaUpdater::installUpdate(ProgressCallback onProgress, void* ctx) {
  if (!isUpdateNewer()) return UPDATE_OLDER_ERROR;

  const esp_partition_t* updatePartition = esp_ota_get_next_update_partition(nullptr);
  if (!updatePartition) {
    LOG_ERR("OTA", "No OTA partition available");
    return INTERNAL_UPDATE_ERROR;
  }

  esp_ota_handle_t otaHandle = 0;
  esp_err_t esp_err = esp_ota_begin(updatePartition, OTA_SIZE_UNKNOWN, &otaHandle);
  if (esp_err != ESP_OK) {
    LOG_ERR("OTA", "esp_ota_begin failed: %s", esp_err_to_name(esp_err));
    return INTERNAL_UPDATE_ERROR;
  }

  esp_wifi_set_ps(WIFI_PS_NONE);

  processedSize = 0;
  int lastReportedPct = -1;
  bool flashOk = true;
  uint8_t hdr[14];
  size_t hdrLen = 0;
  bool wrongChip = false;
  board_tag::Scanner boardScanner;
  bool wrongBoard = false;

  mbedtls_sha256_context shaCtx;
  mbedtls_sha256_init(&shaCtx);
  mbedtls_sha256_starts(&shaCtx, 0);
  uint8_t digest[32];

  const bool fetchOk = HttpDownloader::fetchUrl(
      otaUrl,
      [&](const uint8_t* data, size_t len) {
        mbedtls_sha256_update(&shaCtx, data, len);
        if (hdrLen < sizeof(hdr)) {
          const size_t take = std::min(len, sizeof(hdr) - hdrLen);
          std::memcpy(hdr + hdrLen, data, take);
          hdrLen += take;
          if (hdrLen == sizeof(hdr)) {
            uint16_t imageChip;
            std::memcpy(&imageChip, hdr + 12, sizeof(imageChip));
            const uint16_t deviceChip = firmware_flash::runningPartitionChipId();
            if (deviceChip != 0xFFFF && imageChip != deviceChip) {
              LOG_ERR("OTA", "wrong chip: image=0x%04X device=0x%04X", imageChip, deviceChip);
              wrongChip = true;
              return false;
            }
          }
        }
        boardScanner.feed(data, len);
        if (boardScanner.mismatch()) {
          LOG_ERR("OTA", "wrong board: image=%s device=%.*s", boardScanner.foundName(),
                  static_cast<int>(board_tag::boardNameLen()), board_tag::boardName());
          wrongBoard = true;
          return false;
        }
        if (esp_ota_write(otaHandle, data, len) != ESP_OK) {
          flashOk = false;
          return false;
        }
        processedSize += len;
        if (onProgress && totalSize > 0) {
          const int pct = static_cast<int>(static_cast<uint64_t>(processedSize) * 100 / totalSize);
          if (pct != lastReportedPct) {
            lastReportedPct = pct;
            onProgress(ctx);
          }
        }
        return true;
      },
      kVerifiedTls);

  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
  boardScanner.markStreamComplete();
  mbedtls_sha256_finish(&shaCtx, digest);
  mbedtls_sha256_free(&shaCtx);

  if (wrongChip || wrongBoard || boardScanner.missingRequiredTag()) {
    LOG_ERR("OTA", "Firmware install aborted: wrong or untagged device");
    esp_ota_abort(otaHandle);
    return WRONG_DEVICE_ERROR;
  }

  if (!fetchOk || !flashOk) {
    LOG_ERR("OTA", "Firmware install failed (%s)", flashOk ? "download" : "flash write");
    esp_ota_abort(otaHandle);
    return flashOk ? HTTP_ERROR : INTERNAL_UPDATE_ERROR;
  }

  if (processedSize != otaSize) {
    LOG_ERR("OTA", "Size mismatch: got %zu expected %zu", processedSize, otaSize);
    esp_ota_abort(otaHandle);
    return HASH_MISMATCH_ERROR;
  }

  if (!hexEquals(otaSha256Hex.c_str(), digest)) {
    LOG_ERR("OTA", "SHA256 mismatch");
    esp_ota_abort(otaHandle);
    return HASH_MISMATCH_ERROR;
  }

  uint8_t signingDigest[32];
  if (!otaSigningDigestFromManifest(latestVersion.c_str(), otaBoardTag.c_str(), otaSize, otaSha256Hex.c_str(),
                                    signingDigest)) {
    LOG_ERR("OTA", "Signing payload invalid");
    esp_ota_abort(otaHandle);
    return SIGNATURE_ERROR;
  }

  uint8_t signature[128];
  size_t signatureLen = 0;
  if (!decodeBase64Signature(otaSignatureBase64, signature, sizeof(signature), signatureLen)) {
    LOG_ERR("OTA", "Invalid signature encoding");
    esp_ota_abort(otaHandle);
    return SIGNATURE_ERROR;
  }
  if (!otaVerifyEcdsaP256Sha256(signingDigest, signature, signatureLen, OTA_SIGNING_PUBLIC_KEY_PEM)) {
    LOG_ERR("OTA", "Signature verification failed");
    esp_ota_abort(otaHandle);
    return SIGNATURE_ERROR;
  }

  esp_app_desc_t newAppDesc{};
  if (esp_ota_get_partition_description(updatePartition, &newAppDesc) != ESP_OK) {
    LOG_ERR("OTA", "Failed to read new app descriptor");
    esp_ota_abort(otaHandle);
    return INTERNAL_UPDATE_ERROR;
  }
  if (strncmp(newAppDesc.version, latestVersion.c_str(), sizeof(newAppDesc.version)) != 0) {
    LOG_ERR("OTA", "Image version mismatch: image=%s manifest=%s", newAppDesc.version, latestVersion.c_str());
    esp_ota_abort(otaHandle);
    return WRONG_DEVICE_ERROR;
  }
  if (semverCompareNewer(CROSSPOINT_VERSION, newAppDesc.version) <= 0) {
    LOG_ERR("OTA", "Image version not newer than running");
    esp_ota_abort(otaHandle);
    return UPDATE_OLDER_ERROR;
  }

  esp_err = esp_ota_end(otaHandle);
  if (esp_err != ESP_OK) {
    LOG_ERR("OTA", "esp_ota_end failed: %s", esp_err_to_name(esp_err));
    return INTERNAL_UPDATE_ERROR;
  }

  esp_err = esp_ota_set_boot_partition(updatePartition);
  if (esp_err != ESP_OK) {
    LOG_ERR("OTA", "esp_ota_set_boot_partition failed: %s", esp_err_to_name(esp_err));
    return INTERNAL_UPDATE_ERROR;
  }

  LOG_INF("OTA", "Update completed");
  return OK;
}
