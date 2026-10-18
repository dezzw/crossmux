#include <gtest/gtest.h>

#include "ReaderRefresh.h"

namespace {

TEST(ReaderRefreshCadence, ConsumeRefreshModeDecrementsUntilClean) {
  SETTINGS.refreshFrequency = 15;
  int pages = 5;
  EXPECT_EQ(ReaderUtils::consumeRefreshMode(pages), HalDisplay::FAST_REFRESH);
  EXPECT_EQ(pages, 4);
}

TEST(ReaderRefreshCadence, ConsumeRefreshModeHalfResetsCounter) {
  SETTINGS.refreshFrequency = 15;
  int pages = 1;
  EXPECT_EQ(ReaderUtils::consumeRefreshMode(pages), HalDisplay::HALF_REFRESH);
  EXPECT_EQ(pages, 15);
}

TEST(ReaderRefreshCadence, DueHalfNotDeferredByTransitionFlag) {
  SETTINGS.refreshFrequency = 15;
  int pages = 1;
  EXPECT_EQ(ReaderUtils::consumeRefreshMode(pages, true), HalDisplay::HALF_REFRESH);
  EXPECT_EQ(pages, 15);
}

TEST(ReaderRefreshCadence, DuePageUsesHalfThroughCombinedPath) {
  GfxRenderer renderer;
  renderer.combinedBase_ = true;
  renderer.transition_ = true;
  int pages = 1;
  ReaderUtils::displayBaseWithRefreshCycle(renderer, pages, false);
  EXPECT_EQ(renderer.lastGrayscaleMode, HalDisplay::HALF_REFRESH);
  EXPECT_EQ(pages, 15);
}

TEST(ReaderRefreshCadence, CombinedTextAaPathCallsGrayscaleBase) {
  GfxRenderer renderer;
  renderer.combinedBase_ = true;
  renderer.transition_ = false;
  int pages = 5;
  ReaderUtils::displayBaseWithRefreshCycle(renderer, pages, false);
  EXPECT_EQ(renderer.displayGrayscaleBaseCalls, 1);
  EXPECT_EQ(renderer.displayBufferCalls, 0);
  EXPECT_EQ(renderer.lastGrayscaleMode, HalDisplay::FAST_REFRESH);
  EXPECT_EQ(pages, 4);
}

TEST(ReaderRefreshCadence, NonCombinedPathUsesDisplayBuffer) {
  GfxRenderer renderer;
  renderer.combinedBase_ = false;
  int pages = 5;
  ReaderUtils::displayBaseWithRefreshCycle(renderer, pages, false);
  EXPECT_EQ(renderer.displayGrayscaleBaseCalls, 0);
  EXPECT_EQ(renderer.displayBufferCalls, 1);
}

}  // namespace
