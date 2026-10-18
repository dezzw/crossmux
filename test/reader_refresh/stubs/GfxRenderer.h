#pragma once

#include "DisplayRefreshContext.h"
#include "HalDisplay.h"

class GfxRenderer {
 public:
  mutable int displayGrayscaleBaseCalls = 0;
  mutable int displayBufferCalls = 0;
  mutable HalDisplay::RefreshMode lastGrayscaleMode = HalDisplay::FAST_REFRESH;
  mutable HalDisplay::RefreshMode lastBufferMode = HalDisplay::FAST_REFRESH;

  void displayGrayscaleBase(HalDisplay::RefreshMode mode, DisplayRefreshContext = DisplayRefreshContext::Normal) const {
    ++displayGrayscaleBaseCalls;
    lastGrayscaleMode = mode;
  }

  void displayBuffer(HalDisplay::RefreshMode mode, DisplayRefreshContext = DisplayRefreshContext::Normal) const {
    ++displayBufferCalls;
    lastBufferMode = mode;
  }

  void displayBufferAsync(HalDisplay::RefreshMode mode, DisplayRefreshContext = DisplayRefreshContext::Normal) const {
    displayBuffer(mode);
  }

  void waitRefreshComplete() const {}

  bool supportsTextOnlyCombinedBase() const { return combinedBase_; }
  bool supportsReaderTransitions() const { return transitions_; }
  bool canUseTextTransition() const { return transition_; }

  bool combinedBase_ = true;
  bool transitions_ = true;
  bool transition_ = false;
};
