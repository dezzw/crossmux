#pragma once

#include <cstddef>
#include <cstdint>

// Verifies an ECDSA P-256 SHA-256 signature over a 32-byte digest.
// publicKeyPem must be a PEM-encoded EC public key. signature is raw DER or
// fixed-length P1363 (r||s) — this implementation accepts DER from OpenSSL sign.
bool otaVerifyEcdsaP256Sha256(const uint8_t digest[32], const uint8_t* signature, size_t signatureLen,
                              const char* publicKeyPem);
