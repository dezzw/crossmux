#include <BatteryMonitor.h>
#include <BoardConfig.h>
#include <HalGPIO.h>
#include <Logging.h>
#include <PowerManager.h>
#include <driver/gpio.h>
#include <esp_sleep.h>

#include "Waveshare397Power.h"

#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
#include <soc/usb_serial_jtag_reg.h>
#endif

HalGPIO gpio;

namespace {

#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
uint8_t wavesharePowerButtonHook() {
  return Waveshare397Power::powerButtonPressed() ? static_cast<uint8_t>(1u << HalGPIO::BTN_POWER) : 0;
}

// Keep the last positive USB Serial/JTAG SOF result across nearby polls.
bool usbHostSofActive() {
  static uint32_t lastFrame = 0;
  static unsigned long lastAdvanceMs = 0;
  static bool seeded = false;
  if (!seeded) {
    seeded = true;
    lastFrame = REG_READ(USB_SERIAL_JTAG_FRAM_NUM_REG);
    delay(3);  // A connected host advances the 1 kHz SOF counter within this window.
  }
  const uint32_t frame = REG_READ(USB_SERIAL_JTAG_FRAM_NUM_REG);
  if (frame != lastFrame) {
    lastFrame = frame;
    lastAdvanceMs = millis();
    return true;
  }
  return lastAdvanceMs != 0 && millis() - lastAdvanceMs < 1500;
}
#endif

}  // namespace

void HalGPIO::begin() {
#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
  InputManager::setButtonHook(wavesharePowerButtonHook);
#endif
#if CROSSPOINT_EMULATED == 0
  inputMgr.begin();
#endif
}

void HalGPIO::update() {
  inputMgr.update();
  const bool buttonActivity = inputMgr.wasPressed(BTN_BACK) || inputMgr.wasPressed(BTN_CONFIRM) ||
                              inputMgr.wasPressed(BTN_LEFT) || inputMgr.wasPressed(BTN_RIGHT) ||
                              inputMgr.wasPressed(BTN_UP) || inputMgr.wasPressed(BTN_DOWN);
  const InputModality previous = inputModality.load(std::memory_order_relaxed);
  const InputModality next = inputModalityAfter(previous, buttonActivity, inputMgr.wasTouchActivity());
  inputModalityChanged = next != previous;
  inputModality.store(next, std::memory_order_relaxed);
  const bool connected = isUsbConnected();
  usbStateChanged = (connected != lastUsbConnected);
  lastUsbConnected = connected;
}

bool HalGPIO::wasUsbStateChanged() const { return usbStateChanged; }

bool HalGPIO::isPressed(uint8_t buttonIndex) const { return inputMgr.isPressed(buttonIndex); }

bool HalGPIO::wasPressed(uint8_t buttonIndex) const { return inputMgr.wasPressed(buttonIndex); }

uint8_t HalGPIO::physicalPressedMask() const { return inputMgr.physicalPressedMask(); }

bool HalGPIO::wasAnyPressed() const { return inputMgr.wasAnyPressed(); }

bool HalGPIO::wasReleased(uint8_t buttonIndex) const { return inputMgr.wasReleased(buttonIndex); }

bool HalGPIO::wasAnyReleased() const { return inputMgr.wasAnyReleased(); }

unsigned long HalGPIO::getHeldTime() const { return inputMgr.getHeldTime(); }

unsigned long HalGPIO::getPowerButtonHeldTime() const { return inputMgr.getPowerButtonHeldTime(); }

bool HalGPIO::hasTouch() const { return inputMgr.hasTouch(); }

bool HalGPIO::hasHomeKey() const { return BoardConfig::hasHomeKey(); }

bool HalGPIO::wasHomeKeyTapped() const { return inputMgr.wasHomeKeyTapped(); }

bool HalGPIO::wasHomeKeyLongPressed() const { return inputMgr.wasHomeKeyLongPressed(); }

bool HalGPIO::wasTouchTap(float& nx, float& ny) const { return inputMgr.wasTouchTap(nx, ny); }

bool HalGPIO::wasTouchDown(float& nx, float& ny) const { return inputMgr.wasTouchPressedAt(nx, ny); }

bool HalGPIO::wasTouchReleased() const { return inputMgr.wasTouchReleased(); }

bool HalGPIO::isTouchTapCandidate(float& nx, float& ny, unsigned long& heldMs) const {
  return inputMgr.isTouchTapCandidate(nx, ny, heldMs);
}

bool HalGPIO::isTouchHeldAt(float& nx, float& ny) const { return inputMgr.isTouchHeldAt(nx, ny); }

bool HalGPIO::wasTouchLongPress(float& nx, float& ny) const { return inputMgr.wasTouchLongPress(nx, ny); }

void HalGPIO::suppressTouchContact() { inputMgr.suppressTouchContact(); }

unsigned long HalGPIO::lastTouchHeldMs() const { return inputMgr.lastTouchHeldMs(); }

bool HalGPIO::wasSwipe(float& nxStart, float& nyStart, float& nxEnd, float& nyEnd) const {
  return inputMgr.wasSwipe(nxStart, nyStart, nxEnd, nyEnd);
}

bool HalGPIO::wasTouchActivity() const { return inputMgr.wasTouchActivity(); }

void HalGPIO::clearTouchTapEvent() { inputMgr.clearTouchTapEvent(); }

void HalGPIO::prepareForDeepSleep() { inputMgr.prepareForDeepSleep(); }

bool HalGPIO::restoreTouchAfterDisplayReset() { return inputMgr.reinitializeTouchAfterSharedReset(); }

void HalGPIO::setSharedConfirmPowerShortPressEmitsPower(const bool enabled) {
  InputManager::setSharedConfirmPowerShortPressEmitsPower(enabled);
}

bool HalGPIO::hasEdgeSideButtons() const { return false; }

bool HalGPIO::verifyPowerButtonWakeup() {
  if (BoardConfig::ACTIVE.input.power < 0) {
    return true;
  }

  constexpr unsigned long POWER_WAKE_STABILITY_MS = 10;
  const bool heldAtFirstSample = inputMgr.isPowerButtonPhysicallyPressed();
  const unsigned long sampleStart = millis();
  inputMgr.update();
  while (millis() - sampleStart < POWER_WAKE_STABILITY_MS || inputMgr.isDebouncePending()) {
    delay(1);
    inputMgr.update();
  }
  return heldAtFirstSample && inputMgr.isPowerButtonPhysicallyPressed();
}

bool HalGPIO::verifyPowerButtonWakeup(const uint16_t requiredDurationMs, const bool shortPressAllowed) {
  if (BoardConfig::ACTIVE.input.power < 0 || shortPressAllowed) {
    return true;
  }

  const unsigned long calibration = millis();
  const unsigned long calibratedDuration = calibration < requiredDurationMs ? requiredDurationMs - calibration : 1;
  const unsigned long start = millis();
  inputMgr.update();
  while (!inputMgr.isPressed(BTN_POWER) && millis() - start < 1000) {
    delay(10);
    inputMgr.update();
  }
  if (!inputMgr.isPressed(BTN_POWER)) return false;

  do {
    delay(10);
    inputMgr.update();
  } while (inputMgr.isPressed(BTN_POWER) && inputMgr.getPowerButtonHeldTime() < calibratedDuration);
  return inputMgr.getPowerButtonHeldTime() >= calibratedDuration;
}

bool HalGPIO::isUsbConnected() const {
#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
  bool connected = false;
  if (Waveshare397Power::externalPowerConnected(connected)) return connected;
  return usbHostSofActive();
#else
  if (BoardConfig::ACTIVE.usbDetect >= 0) {
    return digitalRead(BoardConfig::ACTIVE.usbDetect) == HIGH;
  }
  static const BatteryMonitor battery;
  return battery.isCharging();
#endif
}

bool HalGPIO::coldBootImpliesPowerButton() const {
  // Waveshare reports USB/charging through the AXP2101; do not infer a held power
  // button from a no-USB cold boot.
  return false;
}

HalGPIO::WakeupReason HalGPIO::getWakeupReason() const {
  const auto wakeupCause = esp_sleep_get_wakeup_cause();
  const auto resetReason = esp_reset_reason();

  const bool usbConnected = isUsbConnected();

  if (resetReason == ESP_RST_DEEPSLEEP &&
      (wakeupCause == ESP_SLEEP_WAKEUP_GPIO || wakeupCause == ESP_SLEEP_WAKEUP_EXT1)) {
    return WakeupReason::PowerButton;
  }
  if (wakeupCause == ESP_SLEEP_WAKEUP_UNDEFINED && resetReason == ESP_RST_POWERON && !usbConnected &&
      coldBootImpliesPowerButton()) {
    return WakeupReason::PowerButton;
  }
  if (wakeupCause == ESP_SLEEP_WAKEUP_UNDEFINED && resetReason == ESP_RST_UNKNOWN && usbConnected) {
    return WakeupReason::AfterFlash;
  }
  if (wakeupCause == ESP_SLEEP_WAKEUP_UNDEFINED && resetReason == ESP_RST_POWERON && usbConnected) {
    return WakeupReason::AfterUSBPower;
  }
  return WakeupReason::Other;
}
