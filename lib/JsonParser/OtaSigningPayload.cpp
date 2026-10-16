#include "OtaSigningPayload.h"

#include <cctype>
#include <cstdio>
#include <cstring>

#if defined(CROSSPOINT_HOST_TEST) || defined(SIMULATOR)
#include <openssl/evp.h>
#else
#include <mbedtls/sha256.h>
#endif

namespace {

void toLowerHex(char* hex) {
  if (hex == nullptr) return;
  for (char* cursor = hex; *cursor != '\0'; ++cursor) {
    *cursor = static_cast<char>(std::tolower(static_cast<unsigned char>(*cursor)));
  }
}

}  // namespace

bool otaBuildSigningCanonical(char* out, size_t outCapacity, const char* version, const char* boardTag,
                              size_t firmwareSize, const char* sha256Hex) {
  if (out == nullptr || outCapacity == 0 || version == nullptr || boardTag == nullptr || sha256Hex == nullptr) {
    return false;
  }
  char shaLower[65];
  if (strlen(sha256Hex) != 64) return false;
  std::memcpy(shaLower, sha256Hex, 65);
  toLowerHex(shaLower);
  const int written = snprintf(out, outCapacity, "%s|%s|%zu|%s", version, boardTag, firmwareSize, shaLower);
  return written > 0 && static_cast<size_t>(written) < outCapacity;
}

void otaSha256Digest(const uint8_t* data, size_t len, uint8_t digest[32]) {
#if defined(CROSSPOINT_HOST_TEST) || defined(SIMULATOR)
  unsigned int digestLen = 32;
  EVP_Digest(data, len, digest, &digestLen, EVP_sha256(), nullptr);
#else
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(&ctx, data, len);
  mbedtls_sha256_finish(&ctx, digest);
  mbedtls_sha256_free(&ctx);
#endif
}

bool otaSigningDigestFromManifest(const char* version, const char* boardTag, size_t firmwareSize, const char* sha256Hex,
                                  uint8_t digest[32]) {
  char canonical[256];
  if (!otaBuildSigningCanonical(canonical, sizeof(canonical), version, boardTag, firmwareSize, sha256Hex)) {
    return false;
  }
  otaSha256Digest(reinterpret_cast<const uint8_t*>(canonical), strlen(canonical), digest);
  return true;
}
