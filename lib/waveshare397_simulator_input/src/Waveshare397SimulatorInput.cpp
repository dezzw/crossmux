#if defined(SIMULATOR) && defined(SIMULATOR_DEVICE_WAVESHARE_EPAPER_397)

#include "Waveshare397SimulatorInput.h"

#include <Arduino.h>
#include <FunctionButtonGesture.h>
#include <SDL.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "HalGPIO.h"

extern std::atomic<bool> quitRequested;

namespace {

using Gesture = freeink::input::FunctionButtonGesture;

static constexpr int NUM_BUTTONS = 7;
static constexpr SDL_Scancode SIMULATOR_SLEEP_SCANCODE = SDL_SCANCODE_S;

static constexpr SDL_Scancode kBoot = SDL_SCANCODE_ESCAPE;
static constexpr SDL_Scancode kFunction = SDL_SCANCODE_RETURN;
static constexpr SDL_Scancode kDialLeft = SDL_SCANCODE_LEFT;
static constexpr SDL_Scancode kDialRight = SDL_SCANCODE_RIGHT;
static constexpr SDL_Scancode kPower = SDL_SCANCODE_P;

Gesture gesture;
uint8_t logicalDown = 0;
bool pressedThisFrame[NUM_BUTTONS] = {};
bool releasedThisFrame[NUM_BUTTONS] = {};
unsigned long buttonPressStart = 0;
unsigned long buttonPressFinish = 0;
unsigned long powerButtonPressStart = 0;
unsigned long powerButtonPressFinish = 0;
uint8_t physicalPressedThisFrame = 0;
bool syntheticPowerDown = false;
bool simulatorSleepRequested = false;

enum class SyntheticAction { KeyDown, KeyUp, Sleep, Quit };

struct SyntheticEvent {
  unsigned long atMs;
  SyntheticAction action;
  uint8_t rawMask = 0;
  bool handled = false;
};

std::vector<SyntheticEvent> syntheticEvents;
bool syntheticEventsInitialized = false;
uint8_t syntheticRawDown = 0;

uint8_t rawFromKeyboard() {
  const uint8_t* keys = SDL_GetKeyboardState(nullptr);
  uint8_t raw = 0;
  if (keys[kBoot]) raw |= Gesture::BACK;
  if (keys[kFunction]) raw |= Gesture::CONFIRM;
  if (keys[kDialLeft]) raw |= Gesture::LEFT;
  if (keys[kDialRight]) raw |= Gesture::RIGHT;
  raw |= syntheticRawDown;
  return raw;
}

bool powerFromKeyboard() {
  const uint8_t* keys = SDL_GetKeyboardState(nullptr);
  return keys[kPower] || syntheticPowerDown;
}

void applyGestureState(const Gesture::State& state, const unsigned long nowMs) {
  const uint8_t prevDown = logicalDown;
  const bool powerDown = powerFromKeyboard();
  logicalDown = static_cast<uint8_t>(state.down | (powerDown ? (1u << HalGPIO::BTN_POWER) : 0));

  physicalPressedThisFrame = state.physicalPressed;
  for (uint8_t i = 0; i <= HalGPIO::BTN_DOWN; ++i) {
    if (state.pressed & (1u << i)) pressedThisFrame[i] = true;
    if (state.released & (1u << i)) releasedThisFrame[i] = true;
  }

  const bool hadPower = (prevDown & (1u << HalGPIO::BTN_POWER)) != 0;
  const bool hasPower = powerDown;
  if (hasPower && !hadPower) {
    pressedThisFrame[HalGPIO::BTN_POWER] = true;
    powerButtonPressStart = nowMs;
    physicalPressedThisFrame |= static_cast<uint8_t>(1u << HalGPIO::BTN_POWER);
    if (buttonPressStart == 0) buttonPressStart = nowMs;
  }
  if (!hasPower && hadPower) {
    releasedThisFrame[HalGPIO::BTN_POWER] = true;
    powerButtonPressFinish = nowMs;
    buttonPressFinish = nowMs;
  }

  const uint8_t gestureActivity =
      Gesture::LEFT | Gesture::RIGHT | Gesture::UP | Gesture::DOWN | Gesture::BACK | Gesture::CONFIRM;
  if (state.pressed & gestureActivity) buttonPressStart = state.startedMs;
  if (state.released & gestureActivity) buttonPressFinish = nowMs;
}

std::string uppercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return value;
}

uint8_t rawMaskForName(const std::string& name) {
  if (name == "ESCAPE" || name == "BACK" || name == "BOOT") return Gesture::BACK;
  if (name == "RETURN" || name == "ENTER" || name == "CONFIRM" || name == "FUNCTION") return Gesture::CONFIRM;
  if (name == "LEFT") return Gesture::LEFT;
  if (name == "RIGHT") return Gesture::RIGHT;
  if (name == "P" || name == "POWER") return 0;
  return 0;
}

void initializeSyntheticEvents() {
  if (syntheticEventsInitialized) return;
  syntheticEventsInitialized = true;

  const char* script = std::getenv("CROSSPOINT_SIM_INPUT_SCRIPT");
  if (!script || script[0] == '\0') return;

  const std::string spec(script);
  size_t start = 0;
  while (start < spec.size()) {
    const size_t end = spec.find(';', start);
    const std::string item =
        spec.substr(start, end == std::string::npos ? std::string::npos : end - start);
    const size_t firstColon = item.find(':');
    const size_t secondColon =
        firstColon == std::string::npos ? std::string::npos : item.find(':', firstColon + 1);
    if (firstColon != std::string::npos) {
      const unsigned long atMs = std::strtoul(item.substr(0, firstColon).c_str(), nullptr, 10);
      const std::string key = uppercase(item.substr(firstColon + 1, secondColon == std::string::npos
                                                                  ? std::string::npos
                                                                  : secondColon - firstColon - 1));
      if (key == "QUIT") {
        syntheticEvents.push_back({atMs, SyntheticAction::Quit});
      } else if (key == "S" || key == "SLEEP") {
        syntheticEvents.push_back({atMs, SyntheticAction::Sleep});
      } else if (key == "P" || key == "POWER") {
        const unsigned long holdMs = secondColon == std::string::npos
                                         ? 80
                                         : std::strtoul(item.substr(secondColon + 1).c_str(), nullptr, 10);
        syntheticEvents.push_back({atMs, SyntheticAction::KeyDown, 0});
        syntheticEvents.push_back({atMs + holdMs, SyntheticAction::KeyUp, 0});
      } else {
        const uint8_t rawMask = rawMaskForName(key);
        if (rawMask != 0 || key == "P" || key == "POWER") {
          const unsigned long holdMs = secondColon == std::string::npos
                                           ? 80
                                           : std::strtoul(item.substr(secondColon + 1).c_str(), nullptr, 10);
          syntheticEvents.push_back({atMs, SyntheticAction::KeyDown, rawMask});
          syntheticEvents.push_back({atMs + holdMs, SyntheticAction::KeyUp, rawMask});
        }
      }
    }
    if (end == std::string::npos) break;
    start = end + 1;
  }

  std::sort(syntheticEvents.begin(), syntheticEvents.end(),
            [](const SyntheticEvent& a, const SyntheticEvent& b) { return a.atMs < b.atMs; });
}

void processSyntheticEvents() {
  initializeSyntheticEvents();
  const unsigned long now = millis();
  for (auto& event : syntheticEvents) {
    if (event.handled || event.atMs > now) continue;
    event.handled = true;
    switch (event.action) {
      case SyntheticAction::KeyDown:
        if (event.rawMask == 0) {
          syntheticPowerDown = true;
        } else {
          syntheticRawDown |= event.rawMask;
        }
        break;
      case SyntheticAction::KeyUp:
        if (event.rawMask == 0) {
          syntheticPowerDown = false;
        } else {
          syntheticRawDown &= static_cast<uint8_t>(~event.rawMask);
        }
        break;
      case SyntheticAction::Sleep:
        simulatorSleepRequested = true;
        syntheticPowerDown = true;
        break;
      case SyntheticAction::Quit:
        quitRequested.store(true);
        break;
    }
  }
}

void requestSimulatorSleep() {
  simulatorSleepRequested = true;
  syntheticPowerDown = true;
}

}  // namespace

namespace waveshare397_sim {

void begin() {}

void beginFrame() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pressedThisFrame[i] = false;
    releasedThisFrame[i] = false;
  }
  physicalPressedThisFrame = 0;
}

void update() {
  SDL_Event e;
  while (SDL_PollEvent(&e) != 0) {
    if (e.type == SDL_QUIT) {
      quitRequested.store(true);
    } else if (e.type == SDL_KEYDOWN && !e.key.repeat) {
      if (e.key.keysym.scancode == SIMULATOR_SLEEP_SCANCODE) {
        requestSimulatorSleep();
      }
    }
  }
  processSyntheticEvents();

  const unsigned long nowMs = millis();
  const auto state = gesture.update(rawFromKeyboard(), nowMs);
  applyGestureState(state, nowMs);
}

bool isPressed(const uint8_t buttonIndex) {
  if (buttonIndex >= NUM_BUTTONS) return false;
  return (logicalDown & (1u << buttonIndex)) != 0;
}

bool wasPressed(const uint8_t buttonIndex) {
  if (buttonIndex >= NUM_BUTTONS) return false;
  return pressedThisFrame[buttonIndex];
}

bool wasReleased(const uint8_t buttonIndex) {
  if (buttonIndex >= NUM_BUTTONS) return false;
  return releasedThisFrame[buttonIndex];
}

bool wasAnyPressed() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (pressedThisFrame[i]) return true;
  }
  return false;
}

bool wasAnyReleased() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (releasedThisFrame[i]) return true;
  }
  return false;
}

unsigned long getHeldTime() {
  if (logicalDown != 0) return millis() - buttonPressStart;
  return buttonPressFinish - buttonPressStart;
}

unsigned long getPowerButtonHeldTime() {
  if (!isPressed(HalGPIO::BTN_POWER)) return powerButtonPressFinish - powerButtonPressStart;
  return millis() - powerButtonPressStart;
}

uint8_t physicalPressedMask() { return physicalPressedThisFrame; }

bool consumeSimulatorSleepRequest() {
  const bool requested = simulatorSleepRequested;
  simulatorSleepRequested = false;
  return requested;
}

}  // namespace waveshare397_sim

#endif
