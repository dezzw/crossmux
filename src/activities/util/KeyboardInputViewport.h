#pragma once

#include <algorithm>

namespace keyboard_entry {

struct InputLineViewport {
  int firstLineIndex = 0;
  int visibleLineCount = 0;
};

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

inline InputLineViewport viewportForCursor(const int totalLines, const int cursorLineIndex,
                                           const int maxVisibleLines) {
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

inline int cursorLineIndex(const int cursorPos, const int textLength, const int* lineStarts,
                           const int lineStartCount) {
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
