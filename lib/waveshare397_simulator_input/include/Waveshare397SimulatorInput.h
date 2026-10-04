#pragma once

#include <cstdint>

#if defined(SIMULATOR) && defined(SIMULATOR_DEVICE_WAVESHARE_EPAPER_397)

class HalGPIO;

namespace waveshare397_sim {

void begin();
void beginFrame();
void update();
bool isPressed(uint8_t buttonIndex);
bool wasPressed(uint8_t buttonIndex);
bool wasReleased(uint8_t buttonIndex);
bool wasAnyPressed();
bool wasAnyReleased();
unsigned long getHeldTime();
unsigned long getPowerButtonHeldTime();
uint8_t physicalPressedMask();
bool consumeSimulatorSleepRequest();

}  // namespace waveshare397_sim

#endif
