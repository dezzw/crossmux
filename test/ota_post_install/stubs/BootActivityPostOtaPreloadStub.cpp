#include "BootActivityPostOtaPreload.h"

namespace OtaPostInstallTestHost {
int preloadCalls = 0;
}

void postOtaPreloadFontCacheIfNeeded(GfxRenderer&) { ++OtaPostInstallTestHost::preloadCalls; }
