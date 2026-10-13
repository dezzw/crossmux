#include "OtaSignatureVerify.h"

#include <cstring>

#if defined(CROSSPOINT_HOST_TEST) || defined(SIMULATOR)
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#else
#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#endif

bool otaVerifyEcdsaP256Sha256(const uint8_t digest[32], const uint8_t* signature, size_t signatureLen,
                              const char* publicKeyPem) {
  if (digest == nullptr || signature == nullptr || signatureLen == 0 || publicKeyPem == nullptr) return false;

#if defined(CROSSPOINT_HOST_TEST) || defined(SIMULATOR)
  BIO* bio = BIO_new_mem_buf(publicKeyPem, -1);
  if (!bio) return false;
  EVP_PKEY* key = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
  BIO_free(bio);
  if (!key) return false;

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) {
    EVP_PKEY_free(key);
    return false;
  }
  const bool ok = EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, key) == 1 &&
                  EVP_DigestVerify(ctx, signature, signatureLen, digest, 32) == 1;
  EVP_MD_CTX_free(ctx);
  EVP_PKEY_free(key);
  return ok;
#else
  mbedtls_pk_context pk;
  mbedtls_pk_init(&pk);
  const int parsed =
      mbedtls_pk_parse_public_key(&pk, reinterpret_cast<const unsigned char*>(publicKeyPem), strlen(publicKeyPem) + 1);
  if (parsed != 0) {
    mbedtls_pk_free(&pk);
    return false;
  }
  const int verified = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, digest, 32, signature, signatureLen);
  mbedtls_pk_free(&pk);
  return verified == 0;
#endif
}
