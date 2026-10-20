#include "OtaInstallFontGuard.h"

#include "SdCardFontSystem.h"

namespace OtaInstallFontGuard {

void beforeInactiveSlotWrite(GfxRenderer& renderer) { sdFontSystem.releaseLoadedFont(renderer); }

void reloadWithoutFlashCache(GfxRenderer& renderer) { sdFontSystem.ensureLoaded(renderer, false); }

}  // namespace OtaInstallFontGuard
