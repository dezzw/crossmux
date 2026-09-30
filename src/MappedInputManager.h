#pragma once

#include <HalGPIO.h>

#include <array>
#include <climits>
#include <cstdint>

class GfxRenderer;

class MappedInputManager {
 public:
  enum class Button {
    Back,
    Confirm,
    Left,
    Right,
    Up,
    Down,
    Power,
    PageBack,
    PageForward,
    NavNext,
    NavPrevious,
    ScreenLeft,
    ScreenRight,
    ScreenUp,
    ScreenDown,
    Count
  };
  static constexpr uint8_t kButtonCount = static_cast<uint8_t>(Button::Count);
  static_assert(kButtonCount <= 16, "button edge masks use uint16_t");
  enum class SwipeDir { None, Left, Right, Up, Down };

  struct Labels {
    const char* btn1;
    const char* btn2;
    const char* btn3;
    const char* btn4;
  };

  MappedInputManager(HalGPIO& gpio, const GfxRenderer& renderer) : gpio(gpio), renderer(renderer) {}

  void update() const;
  bool wasPressed(Button button) const;
  bool wasReleased(Button button) const;
  bool wasLongPressed(Button button, unsigned long thresholdMs) const;
  bool consumeSuppressedRelease() const;
  bool isPressed(Button button) const;
  bool hasTouch() const;
  bool wasScreenTapped(int& x, int& y) const;
  bool wasScreenTouchDown(int& x, int& y) const;
  bool wasScreenLongPress(int& x, int& y) const;
  bool isScreenTouchHeld(int& x, int& y) const;
  bool wasScreenTouchReleased() const;
  bool wasTapInRect(int x, int y, int width, int height) const;
  bool wasListItemTapped(int& index, int itemCount, int selectedIndex, int listTop, int listHeight,
                         bool hasSubtitle) const;
  bool wasListItemTouchedDown(int& index, int itemCount, int selectedIndex, int listTop, int listHeight,
                              bool hasSubtitle) const;

  enum class RowTouch : uint8_t { None, Down, Tap };
  RowTouch rowTouch(int& row, int top, int rowStep, int rowCount, int xStart = 0, int xEnd = INT32_MAX,
                    int rowHeight = 0) const;
  RowTouch colTouch(int& col, int left, int colStep, int colCount, int yStart, int yEnd, int colWidth = 0) const;

  SwipeDir wasSwipe() const;
  bool wasBackGesture() const;
  bool wasHomeGesture() const;
  bool wasHomeKeyHold() const;
  bool wasMenuGesture() const;
  bool wasReaderMenuSwipeUp() const;
  bool wasAnyPressed() const;
  bool wasAnyReleased() const;
  bool isHeld(const Button button) const;
  unsigned long getHeldTime() const;
  const GfxRenderer& getRenderer() const { return renderer; }
  Labels mapLabels(const char* back, const char* confirm, const char* previous, const char* next) const;
  Labels mapDirectionalLabels(const char* back, const char* confirm, const char* left, const char* right,
                              const char* up, const char* down) const;
  int getPressedFrontButton() const;

  void setBleCaptureMode(bool enabled);
  bool takeCapturedBleKey(uint8_t& kind, uint8_t& value);

  [[nodiscard]] bool isNavDirectionSwapped() const;

 private:
  HalGPIO& gpio;
  const GfxRenderer& renderer;

  Button mapScreenDirection(Button button) const;
  Labels mapFrontLabels(const char* back, const char* confirm, const char* left, const char* right) const;
  bool mapButton(Button button, bool (HalGPIO::*fn)(uint8_t) const) const;
  bool bleEdge(const std::array<bool, kButtonCount>& edges, Button button) const;
  void pollBle() const;
  void suppressNextRelease(Button button) const;

  mutable uint16_t longPressFiredButtons = 0;
  mutable uint16_t suppressedReleaseButtons = 0;
  mutable std::array<bool, kButtonCount> blePressEdges{};
  mutable std::array<bool, kButtonCount> bleReleaseEdges{};
  mutable std::array<bool, kButtonCount> blePendingEdges{};
  mutable bool bleActivityThisFrame = false;
  bool bleCaptureMode = false;
  mutable bool bleHasCaptured = false;
  mutable uint8_t bleCapturedKind = 0xFF;
  mutable uint8_t bleCapturedValue = 0;
};
