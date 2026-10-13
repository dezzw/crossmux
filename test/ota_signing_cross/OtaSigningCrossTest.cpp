#include <gtest/gtest.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

#include <array>
#include <cstdio>
#include <filesystem>
#include <vector>

#include "lib/JsonParser/OtaSignatureVerify.h"
#include "lib/JsonParser/OtaSigningPayload.h"
#include "src/network/OtaSigningTestPublicKey.h"

namespace {

std::string testPrivateKeyPath() {
  return (std::filesystem::path(__FILE__).parent_path().parent_path() / "fixtures" / "ota_test_private.pem").string();
}

EVP_PKEY* testPrivateKey() {
  static EVP_PKEY* cached = []() -> EVP_PKEY* {
    FILE* keyFile = fopen(testPrivateKeyPath().c_str(), "rb");
    if (!keyFile) return nullptr;
    EVP_PKEY* key = PEM_read_PrivateKey(keyFile, nullptr, nullptr, nullptr);
    fclose(keyFile);
    return key;
  }();
  return cached;
}

std::vector<uint8_t> signPrehashedDigest(const uint8_t digest[32]) {
  EVP_PKEY* key = testPrivateKey();
  if (!key) return {};
  EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new(key, nullptr);
  if (!pctx) return {};
  std::vector<uint8_t> signature(128);
  size_t sigLen = signature.size();
  if (EVP_PKEY_sign_init(pctx) != 1 || EVP_PKEY_sign(pctx, signature.data(), &sigLen, digest, 32) != 1) {
    EVP_PKEY_CTX_free(pctx);
    return {};
  }
  signature.resize(sigLen);
  EVP_PKEY_CTX_free(pctx);
  return signature;
}

}  // namespace

TEST(OtaSigningCross, ScriptCanonicalMatchesFirmwareVerify) {
  const char* version = "1.6.0-waveshare-epaper-397-rc.42";
  const char* boardTag = "waveshare_epaper_397";
  const size_t fwSize = 5636288;
  const char* sha256 = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";

  std::array<uint8_t, 32> digest{};
  ASSERT_TRUE(otaSigningDigestFromManifest(version, boardTag, fwSize, sha256, digest.data()));

  const auto signature = signPrehashedDigest(digest.data());
  ASSERT_FALSE(signature.empty());
  EXPECT_TRUE(
      otaVerifyEcdsaP256Sha256(digest.data(), signature.data(), signature.size(), OTA_SIGNING_TEST_PUBLIC_KEY_PEM));
}
