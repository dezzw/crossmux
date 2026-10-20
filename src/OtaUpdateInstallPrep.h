#pragma once

class GfxRenderer;

struct OtaUpdateInstallPrepDeps {
  void (*releaseLoadedFont)(GfxRenderer&) = nullptr;
};

// Unload SD fonts before writing the inactive OTA slot (shared with flash font cache).
void otaReleaseFontsBeforeInactiveSlotWrite(GfxRenderer& renderer,
                                            const OtaUpdateInstallPrepDeps* testDeps = nullptr);
