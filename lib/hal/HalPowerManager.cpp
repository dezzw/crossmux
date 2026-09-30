#include "HalPowerManager.h"

#include <BoardConfig.h>
#include <Logging.h>
#include <PowerManager.h>
#include <WiFi.h>
#include <driver/gpio.h>
#include <esp_sleep.h>

#include <cassert>

#if CONFIG_IDF_TARGET_ESP32C3 && FREEINK_CAP_BLE_HID_HOST
#include <BleKeyboardHost.h>
#endif

#include "HalGPIO.h"
#include "Waveshare397Power.h"

HalPowerManager powerManager;

namespace {
struct StandbyRetention {
  int8_t pin;
  int activeLevel;
};

StandbyRetention standbyRetention() { return {BoardConfig::PIN_UNASSIGNED, LOW}; }
}  // namespace

void HalPowerManager::begin() {
#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
  if (!Waveshare397Power::begin()) LOG_ERR("PWR", "AXP2101 initialization failed");
#endif
  if (BoardConfig::ACTIVE.batteryAdc >= 0) {
    pinMode(BoardConfig::ACTIVE.batteryAdc, INPUT);
  }
  normalFreq = getCpuFrequencyMhz();
  modeMutex = xSemaphoreCreateMutex();
  assert(modeMutex != nullptr);
}

void HalPowerManager::setPowerSaving(bool enabled) {
  if (normalFreq <= 0) {
    return;
  }

  auto wifiMode = WiFi.getMode();
  if (wifiMode != WIFI_MODE_NULL) {
    enabled = false;
  }
#if CONFIG_IDF_TARGET_ESP32C3 && FREEINK_CAP_BLE_HID_HOST
  if (BleHid.isRunning()) enabled = false;
#endif

  xSemaphoreTake(modeMutex, portMAX_DELAY);
  const bool targetLowPower = enabled && currentLockMode == None;
  if (isLowPower != targetLowPower) {
    const int targetFrequency = targetLowPower ? LOW_POWER_FREQ : normalFreq;
    if (setCpuFrequencyMhz(targetFrequency)) {
      isLowPower = targetLowPower;
      LOG_DBG("PWR", "CPU frequency now %u MHz", getCpuFrequencyMhz());
    } else {
      LOG_DBG("PWR", "Failed to set CPU frequency = %d MHz", targetFrequency);
    }
  }

  xSemaphoreGive(modeMutex);
}

void HalPowerManager::startDeepSleep(HalGPIO& gpio) const {
#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
  Waveshare397Power::waitForPowerButtonRelease();
#endif
#if defined(ENABLE_SERIAL_LOG)
  logSerial.end();
#endif

  for (const int8_t pin : {BoardConfig::ACTIVE.power.latch0, BoardConfig::ACTIVE.power.latch1}) {
    if (pin < 0) continue;
    const auto g = static_cast<gpio_num_t>(pin);
    gpio_hold_dis(g);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    gpio_hold_en(g);
  }

  gpio.prepareForDeepSleep();
  freeink::PowerManager::powerDownRailsForSleep();

#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
  if (Waveshare397Power::shutdown()) {
    delay(500);
  } else {
    LOG_ERR("PWR", "AXP2101 shutdown failed; falling back to ESP deep sleep");
  }
  constexpr int8_t FALLBACK_WAKE_PIN = 5;
  pinMode(FALLBACK_WAKE_PIN, INPUT_PULLUP);
  while (digitalRead(FALLBACK_WAKE_PIN) == LOW) delay(50);
  freeink::PowerManager::armWakeOnPins(1ULL << FALLBACK_WAKE_PIN, true);
  freeink::PowerManager::deepSleep();
#endif
}

bool HalPowerManager::canStandbyLightSleep(const HalGPIO& gpio) const {
  (void)gpio;
  return false;
}

HalPowerManager::LightSleepWakeReason HalPowerManager::lightSleepFor(const uint32_t seconds) const {
  const int8_t powerPin = BoardConfig::ACTIVE.input.power;
  const StandbyRetention retention = standbyRetention();
  if (seconds == 0 || powerPin < 0 || retention.pin < 0) {
    LOG_ERR("PWR", "Invalid light-sleep request: seconds=%u powerPin=%d retentionPin=%d",
            static_cast<unsigned>(seconds), powerPin, retention.pin);
    return LightSleepWakeReason::Failed;
  }

  const bool activeHigh = BoardConfig::ACTIVE.input.powerActiveHigh;
  pinMode(powerPin, activeHigh ? INPUT_PULLDOWN : INPUT_PULLUP);
  if (digitalRead(powerPin) == (activeHigh ? HIGH : LOW)) {
    freeink::PowerManager::waitForPowerButtonRelease();
    return LightSleepWakeReason::PowerButton;
  }

  const gpio_num_t retentionPin = static_cast<gpio_num_t>(retention.pin);
  const auto cleanupLightSleep = [&] {
    esp_err_t cleanupError = gpio_wakeup_disable(static_cast<gpio_num_t>(powerPin));
    const auto keepFirstError = [&](const esp_err_t current) {
      if (cleanupError == ESP_OK && current != ESP_OK) cleanupError = current;
    };
    keepFirstError(gpio_set_intr_type(static_cast<gpio_num_t>(powerPin), GPIO_INTR_DISABLE));
    keepFirstError(esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO));
    keepFirstError(esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER));
    keepFirstError(gpio_sleep_sel_en(retentionPin));
    return cleanupError;
  };

  esp_err_t error = gpio_set_level(retentionPin, retention.activeLevel);
  if (error == ESP_OK) error = gpio_set_direction(retentionPin, GPIO_MODE_OUTPUT);
  if (error == ESP_OK) error = gpio_sleep_sel_dis(retentionPin);
  if (error == ESP_OK)
    error =
        gpio_wakeup_enable(static_cast<gpio_num_t>(powerPin), activeHigh ? GPIO_INTR_HIGH_LEVEL : GPIO_INTR_LOW_LEVEL);
  if (error == ESP_OK) error = esp_sleep_enable_gpio_wakeup();
  if (error == ESP_OK) error = esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(seconds) * 1000000ULL);
  if (error != ESP_OK) {
    const esp_err_t cleanupError = cleanupLightSleep();
    LOG_ERR("PWR", "Failed to configure light sleep: %d", static_cast<int>(error));
    if (cleanupError != ESP_OK) LOG_ERR("PWR", "Failed to clean up light sleep: %d", static_cast<int>(cleanupError));
    return LightSleepWakeReason::Failed;
  }

  error = esp_light_sleep_start();
  const esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  const esp_err_t cleanupError = cleanupLightSleep();
  if (cause == ESP_SLEEP_WAKEUP_GPIO) freeink::PowerManager::waitForPowerButtonRelease();
  if (error != ESP_OK) {
    LOG_ERR("PWR", "Light sleep failed: %d", static_cast<int>(error));
    return LightSleepWakeReason::Failed;
  }
  if (cleanupError != ESP_OK) {
    LOG_ERR("PWR", "Failed to clean up light sleep: %d", static_cast<int>(cleanupError));
    return LightSleepWakeReason::Failed;
  }

  switch (cause) {
    case ESP_SLEEP_WAKEUP_TIMER:
      return LightSleepWakeReason::Timer;
    case ESP_SLEEP_WAKEUP_GPIO:
      return LightSleepWakeReason::PowerButton;
    default:
      LOG_ERR("PWR", "Unexpected light-sleep wake cause: %d", static_cast<int>(cause));
      return LightSleepWakeReason::Failed;
  }
}

uint16_t HalPowerManager::getBatteryPercentage() const {
#if FREEINK_DEVICE_WAVESHARE_EPAPER_397
  const unsigned long now = millis();
  if (_batteryLastPollMs != 0 && (now - _batteryLastPollMs) < BATTERY_POLL_MS) return _batteryCachedPercent;
  _batteryLastPollMs = now;
  uint16_t percent = 0;
  if (Waveshare397Power::readBatteryPercentage(percent)) _batteryCachedPercent = percent;
  return _batteryCachedPercent;
#else
  static const BatteryMonitor battery;
  if (BoardConfig::ACTIVE.batteryGauge.gaugeAddr != 0) {
    const unsigned long now = millis();
    if (_batteryLastPollMs != 0 && (now - _batteryLastPollMs) < BATTERY_POLL_MS) {
      return _batteryCachedPercent;
    }

    _batteryLastPollMs = now;
    uint16_t percent = 0;
    if (!battery.readPercentageChecked(percent)) {
      return _batteryCachedPercent;
    }
    _batteryCachedPercent = percent;
    return _batteryCachedPercent;
  }

  if (_batteryCachedPercent == 0) {
    _batteryCachedPercent = 10 * battery.readPercentage();
  } else {
    _batteryCachedPercent = (_batteryCachedPercent * 9 + battery.readPercentage() * 10) / 10;
  }
  return _batteryCachedPercent / 10;
#endif
}

HalPowerManager::Lock::Lock() {
  xSemaphoreTake(powerManager.modeMutex, portMAX_DELAY);
  if (powerManager.currentLockMode != None) {
    LOG_ERR("PWR", "Lock already held, ignore");
    valid = false;
  } else {
    powerManager.currentLockMode = NormalSpeed;
    valid = true;
  }
  xSemaphoreGive(powerManager.modeMutex);
  if (valid) {
    powerManager.setPowerSaving(false);
  }
}

HalPowerManager::Lock::~Lock() {
  xSemaphoreTake(powerManager.modeMutex, portMAX_DELAY);
  if (valid) {
    powerManager.currentLockMode = None;
  }
  xSemaphoreGive(powerManager.modeMutex);
}
