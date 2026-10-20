#pragma once

class GfxRenderer;

namespace OtaInstallFontGuard {

// Unload SD fonts so the inactive OTA slot is not read during esp_ota_begin/write.
void beforeInactiveSlotWrite(GfxRenderer& renderer);

// Reload UI/reader fonts from SD only (flash font cache disabled).
void reloadWithoutFlashCache(GfxRenderer& renderer);

}  // namespace OtaInstallFontGuard
