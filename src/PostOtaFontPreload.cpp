#include "PostOtaFontPreload.h"

#include <HalOtaSlot.h>
#include <Logging.h>
#include <SdCardFontCache.h>

#include "CrossPointSettings.h"
#include "SdCardFontSystem.h"

namespace PostOtaFontPreload {

bool deferUntilHomeConfirm() {
  return HalOtaSlot::runningImageState() == HalOtaSlot::RunningImageState::PendingVerify;
}

void runIfNeeded(GfxRenderer& renderer) {
  if (SETTINGS.sdFontFlashPreload == 0 || SETTINGS.sdFontFamilyName[0] == '\0') return;

  const auto* family = sdFontSystem.registry().findFamily(SETTINGS.sdFontFamilyName);
  const auto* file = family ? family->findNearestSize(SETTINGS.fontPointSize) : nullptr;
  if (!file || SdCardFontCache::isValidFor(file->path.c_str())) return;

  sdFontSystem.releaseLoadedFont(renderer);
  const auto result = SdCardFontCache::preload(file->path.c_str(), nullptr, nullptr);
  sdFontSystem.ensureLoaded(renderer, result == SdCardFontCache::Result::Ok ||
                                           result == SdCardFontCache::Result::AlreadyCached);
  LOG_INF("SDFCACHE", "Post-OTA preload for %s: %s", file->path.c_str(), SdCardFontCache::resultName(result));
}

}  // namespace PostOtaFontPreload
