#pragma once

class GfxRenderer;

class SdCardFontSystem {
 public:
  void releaseLoadedFont(GfxRenderer& renderer);
  void ensureLoaded(GfxRenderer& renderer, bool allowFlashCache = true);
};

extern SdCardFontSystem sdFontSystem;
