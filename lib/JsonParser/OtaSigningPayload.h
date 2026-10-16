#pragma once

#include <cstddef>
#include <cstdint>

/** Canonical OTA signing string: version|boardTag|size|sha256Hex (lowercase). */
bool otaBuildSigningCanonical(char* out, size_t outCapacity, const char* version, const char* boardTag,
                              size_t firmwareSize, const char* sha256Hex);

void otaSha256Digest(const uint8_t* data, size_t len, uint8_t digest[32]);

bool otaSigningDigestFromManifest(const char* version, const char* boardTag, size_t firmwareSize, const char* sha256Hex,
                                  uint8_t digest[32]);
