#include "MappedInputManager.h"

#include <GfxRenderer.h>

#include <algorithm>

#include "BleInput.h"
#include "BleKeyMapping.h"
#include "CrossPointSettings.h"

void MappedInputManager::update() const {
  gpio.update();
#if FREEINK_CAP_BLE_HID_HOST
  BleHid.poll();
  pollBle();
#endif
  for (uint8_t value = 0; value < kButtonCount; ++value) {
    if (!isPressed(static_cast<Button>(value))) longPressFiredButtons &= ~(1u << value);
  }
}

bool MappedInputManager::isNavDirectionSwapped() const {
  const auto orientation = renderer.getOrientation();
  return SETTINGS.frontButtonFollowOrientation &&
         (orientation == GfxRenderer::PortraitInverted || orientation == GfxRenderer::LandscapeCounterClockwise);
}

MappedInputManager::Button MappedInputManager::mapScreenDirection(const Button button) const {
  static constexpr Button directions[][4] = {
      {Button::Left, Button::Right, Button::Up, Button::Down},
      {Button::Down, Button::Up, Button::Left, Button::Right},
      {Button::Right, Button::Left, Button::Down, Button::Up},
      {Button::Up, Button::Down, Button::Right, Button::Left},
  };

  uint8_t direction = 0;
  switch (button) {
    case Button::ScreenLeft:
      direction = 0;
      break;
    case Button::ScreenRight:
      direction = 1;
      break;
    case Button::ScreenUp:
      direction = 2;
      break;
    case Button::ScreenDown:
      direction = 3;
      break;
    default:
      return button;
  }

  const uint8_t orientation =
      SETTINGS.frontButtonFollowOrientation ? static_cast<uint8_t>(renderer.getOrientation()) : 0;
  return directions[orientation][direction];
}

bool MappedInputManager::mapButton(const Button button, bool (HalGPIO::*fn)(uint8_t) const) const {
  const auto sideLayout = SETTINGS.sideButtonLayout;

  switch (button) {
    case Button::Back:
      return (gpio.*fn)(SETTINGS.frontButtonBack);
    case Button::Confirm:
      return (gpio.*fn)(SETTINGS.frontButtonConfirm);
    case Button::Left:
      return (gpio.*fn)(SETTINGS.frontButtonLeft);
    case Button::Right:
      return (gpio.*fn)(SETTINGS.frontButtonRight);
    case Button::Up:
      return (gpio.*fn)(HalGPIO::BTN_UP);
    case Button::Down:
      return (gpio.*fn)(HalGPIO::BTN_DOWN);
    case Button::Power:
      return (gpio.*fn)(HalGPIO::BTN_POWER);
    case Button::PageBack:
      switch (sideLayout) {
        case CrossPointSettings::PREV_NEXT:
          return (gpio.*fn)(isNavDirectionSwapped() ? HalGPIO::BTN_DOWN : HalGPIO::BTN_UP);
        case CrossPointSettings::NEXT_PREV:
          return (gpio.*fn)(isNavDirectionSwapped() ? HalGPIO::BTN_UP : HalGPIO::BTN_DOWN);
        case CrossPointSettings::SIDE_BUTTONS_DISABLED:
        default:
          return false;
      }
    case Button::PageForward:
      switch (sideLayout) {
        case CrossPointSettings::PREV_NEXT:
          return (gpio.*fn)(isNavDirectionSwapped() ? HalGPIO::BTN_UP : HalGPIO::BTN_DOWN);
        case CrossPointSettings::NEXT_PREV:
          return (gpio.*fn)(isNavDirectionSwapped() ? HalGPIO::BTN_DOWN : HalGPIO::BTN_UP);
        case CrossPointSettings::SIDE_BUTTONS_DISABLED:
        default:
          return false;
      }
    case Button::NavNext:
      return isNavDirectionSwapped() ? (mapButton(Button::Up, fn) || mapButton(Button::Left, fn))
                                     : (mapButton(Button::Down, fn) || mapButton(Button::Right, fn));
    case Button::NavPrevious:
      return isNavDirectionSwapped() ? (mapButton(Button::Down, fn) || mapButton(Button::Right, fn))
                                     : (mapButton(Button::Up, fn) || mapButton(Button::Left, fn));
    case Button::ScreenLeft:
    case Button::ScreenRight:
    case Button::ScreenUp:
    case Button::ScreenDown:
      return mapButton(mapScreenDirection(button), fn);
    case Button::Count:
      return false;
  }

  return false;
}

namespace {
MappedInputManager::Button buttonForAction(const bleinput::Action action) {
  using Action = bleinput::Action;
  using Button = MappedInputManager::Button;
  switch (action) {
    case Action::PageForward:
      return Button::PageForward;
    case Action::PageBack:
      return Button::PageBack;
    case Action::Confirm:
      return Button::Confirm;
    case Action::Back:
      return Button::Back;
    case Action::Up:
      return Button::Up;
    case Action::Down:
      return Button::Down;
    case Action::Left:
      return Button::Left;
    case Action::Right:
      return Button::Right;
    case Action::Count:
      return Button::Count;
  }
  return Button::Count;
}
}  // namespace

bool MappedInputManager::bleEdge(const std::array<bool, kButtonCount>& edges, const Button button) const {
  switch (button) {
    case Button::NavNext:
      return isNavDirectionSwapped() ? (bleEdge(edges, Button::Up) || bleEdge(edges, Button::Left))
                                     : (bleEdge(edges, Button::Down) || bleEdge(edges, Button::Right));
    case Button::NavPrevious:
      return isNavDirectionSwapped() ? (bleEdge(edges, Button::Down) || bleEdge(edges, Button::Right))
                                     : (bleEdge(edges, Button::Up) || bleEdge(edges, Button::Left));
    case Button::ScreenLeft:
    case Button::ScreenRight:
    case Button::ScreenUp:
    case Button::ScreenDown:
      return bleEdge(edges, mapScreenDirection(button));
    case Button::Count:
      return false;
    default:
      return edges[static_cast<uint8_t>(button)];
  }
}

void MappedInputManager::pollBle() const {
#if FREEINK_CAP_BLE_HID_HOST
  bleReleaseEdges = blePressEdges;
  bleActivityThisFrame =
      std::any_of(bleReleaseEdges.begin(), bleReleaseEdges.end(), [](const bool edge) { return edge; });
  blePressEdges.fill(false);

  for (uint8_t i = 0; i < kButtonCount; ++i) {
    if (!blePendingEdges[i] || bleReleaseEdges[i]) continue;
    blePendingEdges[i] = false;
    blePressEdges[i] = true;
    bleActivityThisFrame = true;
  }

  freeink::KeyEvent event;
  while (BleHid.popKey(event)) {
    uint8_t kind = 0xFF;
    uint8_t value = 0;
    if (!bleinput::encodeKey(event, kind, value)) continue;
    bleActivityThisFrame = true;
    if (bleCaptureMode) {
      if (!bleHasCaptured) {
        bleCapturedKind = kind;
        bleCapturedValue = value;
        bleHasCaptured = true;
      }
      continue;
    }

    bleinput::Action action;
    if (!bleinput::lookup(SETTINGS.bleKeyMap, kind, value, action)) continue;
    const Button button = buttonForAction(action);
    const uint8_t index = static_cast<uint8_t>(button);
    if (index >= kButtonCount) continue;
    if (blePressEdges[index] || bleReleaseEdges[index]) {
      blePendingEdges[index] = true;
    } else {
      blePressEdges[index] = true;
    }
  }
#endif
}

bool MappedInputManager::hasTouch() const { return false; }

bool MappedInputManager::wasScreenTapped(int& x, int& y) const {
  (void)x;
  (void)y;
  return false;
}

bool MappedInputManager::wasScreenTouchDown(int& x, int& y) const {
  (void)x;
  (void)y;
  return false;
}

bool MappedInputManager::wasScreenLongPress(int& x, int& y) const {
  (void)x;
  (void)y;
  return false;
}

bool MappedInputManager::isScreenTouchHeld(int& x, int& y) const {
  (void)x;
  (void)y;
  return false;
}

bool MappedInputManager::wasScreenTouchReleased() const { return false; }

bool MappedInputManager::wasTapInRect(const int x, const int y, const int width, const int height) const {
  (void)x;
  (void)y;
  (void)width;
  (void)height;
  return false;
}

bool MappedInputManager::wasListItemTapped(int& index, const int itemCount, const int selectedIndex, const int listTop,
                                           const int listHeight, const bool hasSubtitle) const {
  (void)index;
  (void)itemCount;
  (void)selectedIndex;
  (void)listTop;
  (void)listHeight;
  (void)hasSubtitle;
  return false;
}

bool MappedInputManager::wasListItemTouchedDown(int& index, const int itemCount, const int selectedIndex,
                                                const int listTop, const int listHeight, const bool hasSubtitle) const {
  (void)index;
  (void)itemCount;
  (void)selectedIndex;
  (void)listTop;
  (void)listHeight;
  (void)hasSubtitle;
  return false;
}

MappedInputManager::RowTouch MappedInputManager::rowTouch(int& row, const int top, const int rowStep,
                                                          const int rowCount, const int xStart, const int xEnd,
                                                          const int rowHeight) const {
  (void)row;
  (void)top;
  (void)rowStep;
  (void)rowCount;
  (void)xStart;
  (void)xEnd;
  (void)rowHeight;
  return RowTouch::None;
}

MappedInputManager::RowTouch MappedInputManager::colTouch(int& col, const int left, const int colStep,
                                                          const int colCount, const int yStart, const int yEnd,
                                                          const int colWidth) const {
  (void)col;
  (void)left;
  (void)colStep;
  (void)colCount;
  (void)yStart;
  (void)yEnd;
  (void)colWidth;
  return RowTouch::None;
}

MappedInputManager::SwipeDir MappedInputManager::wasSwipe() const { return SwipeDir::None; }

bool MappedInputManager::wasBackGesture() const { return false; }

bool MappedInputManager::wasMenuGesture() const { return false; }

bool MappedInputManager::wasReaderMenuSwipeUp() const { return false; }

bool MappedInputManager::wasHomeGesture() const { return false; }

bool MappedInputManager::wasHomeKeyHold() const { return false; }

bool MappedInputManager::wasPressed(const Button button) const {
  return mapButton(button, &HalGPIO::wasPressed) || bleEdge(blePressEdges, button);
}

bool MappedInputManager::wasReleased(const Button button) const {
  return mapButton(button, &HalGPIO::wasReleased) || bleEdge(bleReleaseEdges, button);
}

bool MappedInputManager::wasLongPressed(const Button button, const unsigned long thresholdMs) const {
  if (!isPressed(button)) return false;
  const uint16_t bit = 1u << static_cast<uint8_t>(button);
  if ((longPressFiredButtons & bit) != 0 || getHeldTime() < thresholdMs) return false;
  longPressFiredButtons |= bit;
  suppressNextRelease(button);
  return true;
}

void MappedInputManager::suppressNextRelease(const Button button) const {
  suppressedReleaseButtons |= 1u << static_cast<uint8_t>(button);
}

bool MappedInputManager::consumeSuppressedRelease() const {
  uint16_t released = 0;
  for (uint8_t value = 0; value < kButtonCount; ++value) {
    const uint16_t bit = 1u << value;
    if ((suppressedReleaseButtons & bit) != 0 && mapButton(static_cast<Button>(value), &HalGPIO::wasReleased)) {
      released |= bit;
    }
  }
  suppressedReleaseButtons &= ~released;
  return released != 0;
}

bool MappedInputManager::isPressed(const Button button) const {
  return mapButton(button, &HalGPIO::isPressed) || bleEdge(blePressEdges, button);
}

bool MappedInputManager::wasAnyPressed() const {
  return gpio.wasAnyPressed() ||
         std::any_of(blePressEdges.begin(), blePressEdges.end(), [](const bool edge) { return edge; });
}

bool MappedInputManager::wasAnyReleased() const {
  return gpio.wasAnyReleased() ||
         std::any_of(bleReleaseEdges.begin(), bleReleaseEdges.end(), [](const bool edge) { return edge; });
}

void MappedInputManager::setBleCaptureMode(const bool enabled) {
  bleCaptureMode = enabled;
  bleHasCaptured = false;
  if (enabled) {
    blePressEdges.fill(false);
    bleReleaseEdges.fill(false);
    blePendingEdges.fill(false);
  }
}

bool MappedInputManager::takeCapturedBleKey(uint8_t& kind, uint8_t& value) {
  if (!bleHasCaptured) return false;
  kind = bleCapturedKind;
  value = bleCapturedValue;
  bleHasCaptured = false;
  return true;
}

unsigned long MappedInputManager::getHeldTime() const {
  if (bleActivityThisFrame) return 0;
  return gpio.getHeldTime();
}

MappedInputManager::Labels MappedInputManager::mapLabels(const char* back, const char* confirm, const char* previous,
                                                         const char* next) const {
  const bool swapLabels = isNavDirectionSwapped();
  const char* leftLabel = swapLabels ? next : previous;
  const char* rightLabel = swapLabels ? previous : next;
  return mapFrontLabels(back, confirm, leftLabel, rightLabel);
}

MappedInputManager::Labels MappedInputManager::mapDirectionalLabels(const char* back, const char* confirm,
                                                                    const char* left, const char* right, const char* up,
                                                                    const char* down) const {
  const auto labelForButton = [&](const Button rawButton) {
    if (mapScreenDirection(Button::ScreenLeft) == rawButton) return left;
    if (mapScreenDirection(Button::ScreenRight) == rawButton) return right;
    if (mapScreenDirection(Button::ScreenUp) == rawButton) return up;
    if (mapScreenDirection(Button::ScreenDown) == rawButton) return down;
    return "";
  };
  return mapFrontLabels(back, confirm, labelForButton(Button::Left), labelForButton(Button::Right));
}

MappedInputManager::Labels MappedInputManager::mapFrontLabels(const char* back, const char* confirm, const char* left,
                                                              const char* right) const {
  auto labelForHardware = [&](uint8_t hw) -> const char* {
    if (hw == SETTINGS.frontButtonBack) return back;
    if (hw == SETTINGS.frontButtonConfirm) return confirm;
    if (hw == SETTINGS.frontButtonLeft) return left;
    if (hw == SETTINGS.frontButtonRight) return right;
    return "";
  };

  return {labelForHardware(HalGPIO::BTN_BACK), labelForHardware(HalGPIO::BTN_CONFIRM),
          labelForHardware(HalGPIO::BTN_LEFT), labelForHardware(HalGPIO::BTN_RIGHT)};
}

int MappedInputManager::getPressedFrontButton() const {
  if (gpio.wasPressed(HalGPIO::BTN_BACK)) return HalGPIO::BTN_BACK;
  if (gpio.wasPressed(HalGPIO::BTN_CONFIRM)) return HalGPIO::BTN_CONFIRM;
  if (gpio.wasPressed(HalGPIO::BTN_LEFT)) return HalGPIO::BTN_LEFT;
  if (gpio.wasPressed(HalGPIO::BTN_RIGHT)) return HalGPIO::BTN_RIGHT;
  return -1;
}

bool MappedInputManager::isHeld(const Button button) const { return isPressed(button); }
