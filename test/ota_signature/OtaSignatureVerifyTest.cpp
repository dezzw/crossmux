#include <gtest/gtest.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

#include <array>
#include <cstdio>
#include <filesystem>
#include <vector>

#include "lib/JsonParser/OtaSignatureVerify.h"
#include "src/network/OtaSigningTestPublicKey.h"

namespace {

std::string testPrivateKeyPath() {
  return (std::filesystem::path(__FILE__).parent_path().parent_path() / "fixtures" / "ota_test_private.pem").string();
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

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  std::vector<uint8_t> signature(128);
  size_t sigLen = signature.size();
  if (EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, key) != 1 ||
      EVP_DigestSign(ctx, signature.data(), &sigLen, digest, 32) != 1) {
    EVP_MD_CTX_free(ctx);
    return {};
  }
  signature.resize(sigLen);
  EVP_MD_CTX_free(ctx);
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
