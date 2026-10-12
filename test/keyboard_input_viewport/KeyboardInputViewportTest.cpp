#include <gtest/gtest.h>

#include "activities/util/KeyboardInputViewport.h"

TEST(KeyboardInputViewport, MaxVisibleLinesRespectsKeyboardTop) {
  EXPECT_EQ(keyboard_entry::maxVisibleInputLines(100, 400, 20, 8, 0), 13);
  EXPECT_EQ(keyboard_entry::maxVisibleInputLines(100, 140, 20, 8, 0), 1);
  EXPECT_EQ(keyboard_entry::maxVisibleInputLines(100, 400, 20, 8, 80), 9);
}

TEST(KeyboardInputViewport, ViewportScrollsToKeepCursorVisible) {
  const auto all = keyboard_entry::viewportForCursor(10, 0, 3);
  EXPECT_EQ(all.firstLineIndex, 0);
  EXPECT_EQ(all.visibleLineCount, 3);

  const auto end = keyboard_entry::viewportForCursor(10, 9, 3);
  EXPECT_EQ(end.firstLineIndex, 7);
  EXPECT_EQ(end.visibleLineCount, 3);

  const auto mid = keyboard_entry::viewportForCursor(10, 5, 4);
  EXPECT_EQ(mid.firstLineIndex, 2);
  EXPECT_EQ(mid.visibleLineCount, 4);
  EXPECT_GE(5, mid.firstLineIndex);
  EXPECT_LT(5, mid.firstLineIndex + mid.visibleLineCount);
}

TEST(KeyboardInputViewport, CursorLineIndexFromStarts) {
  const int starts[] = {0, 10, 25, 40};
  EXPECT_EQ(keyboard_entry::cursorLineIndex(0, 50, starts, 4), 0);
  EXPECT_EQ(keyboard_entry::cursorLineIndex(10, 50, starts, 4), 1);
  EXPECT_EQ(keyboard_entry::cursorLineIndex(40, 50, starts, 4), 3);
}

TEST(KeyboardInputViewport, TipsVisibleBandReserveMatchesSixLinesPlusSpacing) {
  constexpr int tipsLh = 18;
  constexpr int spacing = 8;
  const int reserve =
      keyboard_entry::keyInstructionBandHeightPx(tipsLh, spacing, keyboard_entry::worstCaseKeyInstructionBandLayout());
  EXPECT_EQ(reserve, tipsLh * 6 + spacing);

  const int withTips = keyboard_entry::maxVisibleInputLines(100, 400, 20, spacing, reserve);
  const int oldUnderserved = keyboard_entry::maxVisibleInputLines(100, 400, 20, spacing, tipsLh * 4 + spacing);
  EXPECT_LT(withTips, oldUnderserved);

  const int underlineBottom = 100 + (withTips - 1) * 20 + 20 + spacing + 4;
  EXPECT_LE(underlineBottom + reserve, 400);
}
