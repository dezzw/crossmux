#include <gtest/gtest.h>

#include <array>
#include <cstdlib>
#include <vector>

#include "lib/JsonParser/OtaSignatureVerify.h"
#include "lib/JsonParser/OtaSigningPayload.h"
#include "ota_signing_known_answer.generated.h"

namespace {

std::array<uint8_t, 32> hexToBytes(const char* hex) {
  std::array<uint8_t, 32> out{};
  for (size_t i = 0; i < out.size(); ++i) {
    char pair[3] = {hex[i * 2], hex[i * 2 + 1], '\0'};
    out[i] = static_cast<uint8_t>(strtoul(pair, nullptr, 16));
  }
  return out;
}

std::vector<uint8_t> hexToSig(const char* hex) {
  const size_t len = strlen(hex) / 2;
  std::vector<uint8_t> out(len);
  for (size_t i = 0; i < len; ++i) {
    char pair[3] = {hex[i * 2], hex[i * 2 + 1], '\0'};
    out[i] = static_cast<uint8_t>(strtoul(pair, nullptr, 16));
  }
  return out;
}

}  // namespace

TEST(OtaSignatureKnownAnswer, SignScriptVectorVerifiesWithMbedtls) {
  std::array<uint8_t, 32> digest{};
  ASSERT_TRUE(otaSigningDigestFromManifest(OTA_KNOWN_VERSION, OTA_KNOWN_BOARD, OTA_KNOWN_FIRMWARE_SIZE,
                                           OTA_KNOWN_SHA256, digest.data()));
  EXPECT_EQ(hexToBytes(OTA_KNOWN_DIGEST_HEX), digest);

  const auto signature = hexToSig(OTA_KNOWN_SIGNATURE_HEX);
  ASSERT_FALSE(signature.empty());
  EXPECT_TRUE(otaVerifyEcdsaP256Sha256(digest.data(), signature.data(), signature.size(), OTA_KNOWN_PUBLIC_KEY_PEM));
}
