#pragma once

#include <algorithm>

namespace keyboard_entry {

struct InputLineViewport {
  int firstLineIndex = 0;
  int visibleLineCount = 0;
};

struct KeyInstructionBandLayout {
  int tipCountBelowHeader = 0;
  int inlineHintLines = 0;
};

// Matches KeyboardEntryActivity::render() tipCount branches.
inline int tipCountBelowHeader(const bool cursorMode, const bool urlPanel, const bool symbols, const bool textEmpty,
                               const bool isUrlInput) {
  if (cursorMode) return 1;
  if (urlPanel) return 1 + (textEmpty ? 0 : 1);
  if (symbols) return textEmpty ? 0 : 1;
  return 1 + (isUrlInput ? 1 : 0) + (textEmpty ? 0 : 1);
}

inline int inlineHintLineCount(const bool keyTipsVisible, const bool hintVisible, const bool textEmpty,
                               const bool cursorMode) {
  if (!keyTipsVisible || !hintVisible || textEmpty) return 0;
  return cursorMode ? 2 : 1;
}

inline KeyInstructionBandLayout worstCaseKeyInstructionBandLayout() {
  return {tipCountBelowHeader(false, false, false, false, true), 2};
}

// Vertical space below the input underline reserved for inline hints plus the centered tip block.
inline int keyInstructionBandHeightPx(const int tipsLineHeight, const int verticalSpacing,
                                      const KeyInstructionBandLayout layout) {
  if (tipsLineHeight <= 0) return 0;
  if (layout.tipCountBelowHeader <= 0 && layout.inlineHintLines <= 0) return 0;
  const int mainBlockLines = layout.tipCountBelowHeader > 0 ? (1 + layout.tipCountBelowHeader) : 0;
  const int totalLines = layout.inlineHintLines + mainBlockLines;
  return tipsLineHeight * totalLines + verticalSpacing;
}

// Lines of input text must fit between inputTop and keyboardTop, including one underline row.
inline int maxVisibleInputLines(const int inputTop, const int keyboardTop, const int lineHeight,
                                const int verticalSpacing, const int tipBandReserve = 0) {
  if (lineHeight <= 0) return 1;
  constexpr int kUnderlinePad = 4;
  const int reserved = lineHeight + verticalSpacing + kUnderlinePad + std::max(0, tipBandReserve);
  const int available = keyboardTop - inputTop - reserved;
  if (available < lineHeight) return 1;
  return std::max(1, available / lineHeight);
}

inline InputLineViewport viewportForCursor(const int totalLines, const int cursorLineIndex, const int maxVisibleLines) {
  const int maxVis = std::max(1, maxVisibleLines);
  if (totalLines <= 0) {
    return {0, 0};
  }
  if (totalLines <= maxVis) {
    return {0, totalLines};
  }
  int first = cursorLineIndex - maxVis + 1;
  if (first < 0) first = 0;
  if (first + maxVis > totalLines) first = totalLines - maxVis;
  if (cursorLineIndex < first) first = cursorLineIndex;
  if (cursorLineIndex >= first + maxVis) first = cursorLineIndex - maxVis + 1;
  return {first, maxVis};
}

inline int cursorLineIndex(const int cursorPos, const int textLength, const int* lineStarts, const int lineStartCount) {
  if (lineStartCount <= 0) return 0;
  int line = 0;
  for (int i = lineStartCount - 1; i >= 0; --i) {
    if (cursorPos >= lineStarts[i]) {
      line = i;
      break;
    }
  }
  const int lineEnd = (line + 1 < lineStartCount) ? lineStarts[line + 1] : textLength;
  if (cursorPos > lineEnd) return lineStartCount - 1;
  return line;
}

}  // namespace keyboard_entry
