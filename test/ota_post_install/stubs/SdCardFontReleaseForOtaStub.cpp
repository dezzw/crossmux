#include "SdCardFontReleaseForOta.h"

namespace OtaPostInstallTestHost {
int releaseLoadedFontCalls = 0;
}

void sdCardFontReleaseLoadedForOta(GfxRenderer&) { ++OtaPostInstallTestHost::releaseLoadedFontCalls; }
