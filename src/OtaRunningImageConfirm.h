#pragma once

#include <HalOtaSlot.h>

class GfxRenderer;

struct OtaRunningImageConfirmDeps {
  HalOtaSlot::RunningImageState (*runningImageState)() = nullptr;
  bool (*confirmRunningImage)() = nullptr;
  void (*preloadFontCacheIfNeeded)(GfxRenderer&) = nullptr;
};

// Idempotent: confirms a pending-verify running image once UI/storage are ready.
void otaConfirmPendingRunningImageAtHome(GfxRenderer& renderer, const OtaRunningImageConfirmDeps* testDeps = nullptr);
