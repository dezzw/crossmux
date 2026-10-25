#include <gtest/gtest.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

#include <array>
#include <cstdio>
#include <vector>

#include "OtaSigningTestPublicKey.h"
#include "lib/JsonParser/OtaSignatureVerify.h"

namespace {

std::string testPrivateKeyPath() {
#ifdef CROSSPOINT_OTA_TEST_PRIVATE_KEY_PATH
  return CROSSPOINT_OTA_TEST_PRIVATE_KEY_PATH;
#else
#error "CROSSPOINT_OTA_TEST_PRIVATE_KEY_PATH must be set for OTA host tests"
#endif
}

EVP_PKEY* testPrivateKey() {
  static EVP_PKEY* cached = []() -> EVP_PKEY* {
    const std::string path = testPrivateKeyPath();
    FILE* keyFile = fopen(path.c_str(), "rb");
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

TEST(OtaSignatureVerify, GoodSignature) {
  std::array<uint8_t, 32> digest{};
  digest.fill(0xAB);
  const auto signature = signPrehashedDigest(digest.data());
  ASSERT_FALSE(signature.empty());
  EXPECT_TRUE(
      otaVerifyEcdsaP256Sha256(digest.data(), signature.data(), signature.size(), OTA_SIGNING_TEST_PUBLIC_KEY_PEM));
}

TEST(OtaSignatureVerify, BadSignature) {
  std::array<uint8_t, 32> digest{};
  digest.fill(0xAB);
  std::array<uint8_t, 32> badSig{};
  badSig.fill(0x01);
  EXPECT_FALSE(otaVerifyEcdsaP256Sha256(digest.data(), badSig.data(), badSig.size(), OTA_SIGNING_TEST_PUBLIC_KEY_PEM));
}

TEST(OtaSignatureVerify, BadDigest) {
  std::array<uint8_t, 32> digest{};
  digest.fill(0xAB);
  const auto signature = signPrehashedDigest(digest.data());
  ASSERT_FALSE(signature.empty());
  digest[0] ^= 0xFF;
  EXPECT_FALSE(
      otaVerifyEcdsaP256Sha256(digest.data(), signature.data(), signature.size(), OTA_SIGNING_TEST_PUBLIC_KEY_PEM));
}
