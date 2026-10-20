#include "OtaUpdateInstallPrep.h"

#include "SdCardFontReleaseForOta.h"

void otaReleaseFontsBeforeInactiveSlotWrite(GfxRenderer& renderer, const OtaUpdateInstallPrepDeps* testDeps) {
  if (testDeps && testDeps->releaseLoadedFont) {
    testDeps->releaseLoadedFont(renderer);
    return;
  }
  sdCardFontReleaseLoadedForOta(renderer);
}
