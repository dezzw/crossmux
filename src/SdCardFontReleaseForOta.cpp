#include "SdCardFontReleaseForOta.h"

#include "SdCardFontSystem.h"

void sdCardFontReleaseLoadedForOta(GfxRenderer& renderer) { sdFontSystem.releaseLoadedFont(renderer); }
