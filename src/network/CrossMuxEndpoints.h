#pragma once

#include <cstddef>
#include <cstdio>

#define FONT_MANIFEST_VER_STRING_INNER(x) #x
#define FONT_MANIFEST_VER_STRING(x) FONT_MANIFEST_VER_STRING_INNER(x)

namespace CrossMuxEndpoints {

// OTA and stable releases for this fork.
inline constexpr char OTA_GITHUB_REPO[] = "dezzw/crossmux";
// Upstream SD font bundles (primary manifest source).
inline constexpr char FONT_GITHUB_REPO[] = "crosspoint-reader/crosspoint-fonts";

inline constexpr char LEGACY_HOST[] = "crossmux.com";
inline constexpr char FONT_LEGACY_MANIFEST_FORMAT[] = "https://%s/api/assets/fonts/m%s-b%s/fonts.json";
inline constexpr char FONT_GITHUB_MANIFEST_FORMAT[] =
    "https://github.com/%s/releases/download/sd-fonts-m%s-b%s/fonts.json";

inline constexpr char OTA_STABLE_MANIFEST_FORMAT[] =
    "https://github.com/%s/releases/latest/download/%s-global-manifest.json";
inline constexpr char OTA_NIGHTLY_MANIFEST_FORMAT[] =
    "https://github.com/%s/releases/download/nightly/%s-global-manifest.json";

#if defined(FREEINK_DEVICE_WAVESHARE_EPAPER_397)
inline constexpr char OTA_DEVICE_SLUG[] = "waveshare-epaper-397";
#elif defined(CROSSPOINT_EMULATED)
inline constexpr char OTA_DEVICE_SLUG[] = "waveshare-epaper-397";
#else
#error "CrossMuxEndpoints: unsupported device"
#endif

inline constexpr const char* otaDeviceSlug() { return OTA_DEVICE_SLUG; }

inline int formatFontGithubManifestUrl(char* buffer, size_t capacity, int manifestVersion, int cpfontVersion) {
  return snprintf(buffer, capacity, FONT_GITHUB_MANIFEST_FORMAT, FONT_GITHUB_REPO,
                  FONT_MANIFEST_VER_STRING(manifestVersion), FONT_MANIFEST_VER_STRING(cpfontVersion));
}

inline int formatFontLegacyManifestUrl(char* buffer, size_t capacity, int manifestVersion, int cpfontVersion) {
  return snprintf(buffer, capacity, FONT_LEGACY_MANIFEST_FORMAT, LEGACY_HOST, FONT_MANIFEST_VER_STRING(manifestVersion),
                  FONT_MANIFEST_VER_STRING(cpfontVersion));
}

inline int formatOtaStableManifestUrl(char* buffer, size_t capacity) {
  return snprintf(buffer, capacity, OTA_STABLE_MANIFEST_FORMAT, OTA_GITHUB_REPO, OTA_DEVICE_SLUG);
}

inline int formatOtaNightlyManifestUrl(char* buffer, size_t capacity) {
  return snprintf(buffer, capacity, OTA_NIGHTLY_MANIFEST_FORMAT, OTA_GITHUB_REPO, OTA_DEVICE_SLUG);
}

inline constexpr const char* legacyHost() { return LEGACY_HOST; }
inline constexpr char DICTIONARY_MANIFEST_FORMAT[] = "https://%s/api/assets/dictionaries/manifest?version=1&lang=%s";

}  // namespace CrossMuxEndpoints
