#pragma once

#include <Arduino.h>
#include <InputManager.h>

#include <atomic>

#include "InputModality.h"

// Display SPI pins (Waveshare ESP32-S3 ePaper 3.97)
#define EPD_SCLK 8   // SPI Clock
#define EPD_MOSI 10  // SPI MOSI (Master Out Slave In)
#define EPD_CS 21    // Chip Select
#define EPD_DC 4     // Data/Command
#define EPD_RST 5    // Reset
#define EPD_BUSY 6   // Busy

#define SPI_MISO 7  // SPI MISO, shared between SD card and display (Master In Slave Out)

#define BAT_GPIO0 0  // Battery voltage

#define UART0_RXD 20  // Used for USB connection detection

class HalGPIO {
#if CROSSPOINT_EMULATED == 0
  InputManager inputMgr;
#endif

  bool lastUsbConnected = false;
  bool usbStateChanged = false;
  std::atomic<InputModality> inputModality{InputModality::Touch};
  bool inputModalityChanged = false;

 public:
  HalGPIO() = default;

  // True when the board's page buttons sit on the left/right screen edges rather
  // than an off-screen vertical rocker.
  bool hasEdgeSideButtons() const;

  void begin();

  void update();
  bool isPressed(uint8_t buttonIndex) const;
  bool wasPressed(uint8_t buttonIndex) const;
  uint8_t physicalPressedMask() const;
  bool wasAnyPressed() const;
  bool wasReleased(uint8_t buttonIndex) const;
  bool wasAnyReleased() const;
  unsigned long getHeldTime() const;
  unsigned long getPowerButtonHeldTime() const;
  bool hasTouch() const;
  bool hasHomeKey() const;
  bool wasHomeKeyTapped() const;
  bool wasHomeKeyLongPressed() const;
  bool wasTouchTap(float& nx, float& ny) const;
  bool wasTouchDown(float& nx, float& ny) const;
  bool wasTouchReleased() const;
  bool isTouchTapCandidate(float& nx, float& ny, unsigned long& heldMs) const;
  bool isTouchHeldAt(float& nx, float& ny) const;
  bool wasTouchLongPress(float& nx, float& ny) const;
  void suppressTouchContact();
  unsigned long lastTouchHeldMs() const;
  bool wasSwipe(float& nxStart, float& nyStart, float& nxEnd, float& nyEnd) const;
  bool wasTouchActivity() const;
  InputModality lastInputModality() const { return inputModality.load(std::memory_order_relaxed); }
  bool wasInputModalityChanged() const { return inputModalityChanged; }
  void clearTouchTapEvent();
  void prepareForDeepSleep();
  bool restoreTouchAfterDisplayReset();
  void setSharedConfirmPowerShortPressEmitsPower(bool enabled);

  bool verifyPowerButtonWakeup();
  bool verifyPowerButtonWakeup(uint16_t requiredDurationMs, bool shortPressAllowed);

  bool isUsbConnected() const;

  // When false, cold boots always proceed to a normal boot (no held-button inference).
  bool coldBootImpliesPowerButton() const;

  bool wasUsbStateChanged() const;

  enum class WakeupReason { PowerButton, AfterFlash, AfterUSBPower, Other };

  WakeupReason getWakeupReason() const;

  static constexpr uint8_t BTN_BACK = 0;
  static constexpr uint8_t BTN_CONFIRM = 1;
  static constexpr uint8_t BTN_LEFT = 2;
  static constexpr uint8_t BTN_RIGHT = 3;
  static constexpr uint8_t BTN_UP = 4;
  static constexpr uint8_t BTN_DOWN = 5;
  static constexpr uint8_t BTN_POWER = 6;
};

extern HalGPIO gpio;
