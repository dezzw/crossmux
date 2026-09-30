---
name: port-device-bsp
description: >-
  Waveshare ESP32-S3 ePaper 3.97 BSP work in this fork: FreeInk SDK board
  profile, CrossMux HAL wiring, build env, and recorded hardware acceptance.
  Use for panel/timing fixes, pin or PMIC changes, or SDK submodule updates on
  this board—not for adding other hardware targets.
---

# Waveshare BSP (this fork)

This repository ships **one** hardware target: Waveshare ESP32-S3 ePaper 3.97
(`waveshare_epaper_397`). There is no multi-board onboarding path here; extend
the existing profile and HAL instead of adding new `FREEINK_DEVICE_*` targets.

## Before you change anything

1. Read [AGENTS.md](../../../AGENTS.md) and
   [waveshare-epaper-397.md](../../../docs/engineering/waveshare-epaper-397.md).
2. Confirm the pinned `freeink-sdk` submodule is initialized
   (`git submodule update --init --recursive`). Do not bump the SDK with
   `--remote` unless the task explicitly requires it.
3. Cite file/line evidence for pin maps, refresh sequences, and button mapping.

## Layers

| Layer | Responsibility |
|---|---|
| SDK | Board config, SSD1677 Waveshare LUT, AXP2101/RTC/audio drivers |
| HAL | `HalGPIO`, display, storage, power—no direct SDK calls from activities |
| App | Logical buttons via `MappedInputManager`; rendering via `GUI` |
| Build | `platformio.ini` env `waveshare_epaper_397`; see [build-system.md](../../../docs/engineering/build-system.md) |

Memory budgeting: [hardware-constraints.md](../../../docs/engineering/hardware-constraints.md)
(internal SRAM first; PSRAM for framebuffer/large buffers only where enabled).

## Bring-up checklist (hardware)

Record **passed / failed / pending** with commands and logs:

- Boot without panic/OOM; SD mounts; identity matches Waveshare profile.
- FULL/HALF/FAST and reading grayscale path; no stuck BUSY without recovery.
- Face buttons and Power: one gesture → one action; Function double-click/hold
  matches [waveshare-epaper-397.md](../../../docs/engineering/waveshare-epaper-397.md).
- Open EPUB, turn pages, reboot—progress and settings persist.
- Sleep/wake (AXP2101 power key); optional sound feedback levels if touched.
- Heap stable after repeated reading and sleep cycles (internal + PSRAM if used).

Build: `pio run -e waveshare_epaper_397`. Code changes: `./bin/ci-check`.
Serial: `python3 scripts/debugging_monitor.py <port>`.

## Handoff

1. Hardware facts and SDK/HAL diffs with evidence.
2. Verification table (command, log, or measurement per row).
3. Update [waveshare-epaper-397.md](../../../docs/engineering/waveshare-epaper-397.md)
   when behavior or acceptance status changes—not a new per-board doc tree.

PR scope (only when asked): focused branch, target `0x1abin/crossmux:main`, SDK
changes in a separate PR if the submodule must move; keep Draft until flash
acceptance is recorded.
