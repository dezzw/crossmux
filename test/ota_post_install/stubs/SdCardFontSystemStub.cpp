#include "SdCardFontSystem.h"

SdCardFontSystem sdFontSystem;

namespace OtaPostInstallTestHost {
int releaseLoadedFontCalls = 0;
int ensureLoadedCalls = 0;
bool lastEnsureAllowFlash = true;
}  // namespace OtaPostInstallTestHost

void SdCardFontSystem::releaseLoadedFont(GfxRenderer&) { ++OtaPostInstallTestHost::releaseLoadedFontCalls; }

void SdCardFontSystem::ensureLoaded(GfxRenderer&, bool allowFlashCache) {
  ++OtaPostInstallTestHost::ensureLoadedCalls;
  OtaPostInstallTestHost::lastEnsureAllowFlash = allowFlashCache;
}
