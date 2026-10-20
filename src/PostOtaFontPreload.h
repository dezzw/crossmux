#pragma once

class GfxRenderer;

namespace PostOtaFontPreload {

// True while the running OTA image still needs rollback confirmation.
bool deferUntilHomeConfirm();

// Populate the inactive-slot font cache when settings allow. No UI; logs outcome.
// Safe to call after home confirmation (scratch writes allowed).
void runIfNeeded(GfxRenderer& renderer);

}  // namespace PostOtaFontPreload
