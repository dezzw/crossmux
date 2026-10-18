#pragma once

#include <cstddef>
#include <cstdio>

namespace CrossMuxEndpoints {

inline constexpr char OTA_GITHUB_REPO[] = "dezzw/crossmux";
inline constexpr char FONT_GITHUB_REPO[] = "dezzw/crossmux";

inline constexpr char FONT_GITHUB_MANIFEST_FORMAT[] =
    "https://github.com/%s/releases/download/fonts-m%d-b%d/fonts.json";

inline constexpr char OTA_STABLE_MANIFEST_FORMAT[] =
    "https://github.com/%s/releases/download/stable/%s-global-manifest.json";

#if defined(FREEINK_DEVICE_WAVESHARE_EPAPER_397)
inline constexpr char OTA_DEVICE_SLUG[] = "waveshare-epaper-397";
#elif defined(CROSSPOINT_EMULATED)
inline constexpr char OTA_DEVICE_SLUG[] = "waveshare-epaper-397";
#else
#error "CrossMuxEndpoints: unsupported device"
#endif

inline constexpr const char* otaDeviceSlug() { return OTA_DEVICE_SLUG; }

inline int formatFontGithubManifestUrl(char* buffer, size_t capacity, int manifestVersion, int cpfontVersion) {
  return snprintf(buffer, capacity, FONT_GITHUB_MANIFEST_FORMAT, FONT_GITHUB_REPO, manifestVersion, cpfontVersion);
}

inline int formatOtaStableManifestUrl(char* buffer, size_t capacity) {
  return snprintf(buffer, capacity, OTA_STABLE_MANIFEST_FORMAT, OTA_GITHUB_REPO, OTA_DEVICE_SLUG);
}

<<<<<<< HEAD
inline constexpr const char* legacyHost() { return LEGACY_HOST; }
inline constexpr char DICTIONARY_MANIFEST_FORMAT[] = "https://%s/api/assets/dictionaries/manifest?version=1&lang=%s";
=======
inline int formatOtaNightlyManifestUrl(char* buffer, size_t capacity) {
  return snprintf(buffer, capacity, OTA_NIGHTLY_MANIFEST_FORMAT, OTA_GITHUB_REPO, OTA_DEVICE_SLUG);
}
>>>>>>> 7db6b1ae ([refactor] Remove crossmux.com dependency and dictionary OTA path)

}  // namespace CrossMuxEndpoints
