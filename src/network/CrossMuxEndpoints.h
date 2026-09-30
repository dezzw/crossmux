#pragma once

namespace CrossMuxEndpoints {

inline constexpr char GLOBAL_HOST[] = "crossmux.com";
inline constexpr char OTA_MANIFEST_FORMAT[] = "https://%s/api/ota/manifest?variant=%s%s&model=%s";
inline constexpr char FONT_MANIFEST_FORMAT[] = "https://%s/api/assets/fonts/m%s-b%s/fonts.json";
inline constexpr char DICTIONARY_MANIFEST_FORMAT[] = "https://%s/api/assets/dictionaries/manifest?version=1&lang=%s";

inline constexpr const char* host() { return GLOBAL_HOST; }
inline constexpr const char* otaVariant() { return "global"; }

}  // namespace CrossMuxEndpoints
