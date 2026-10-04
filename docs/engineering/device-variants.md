# Device Variants — Waveshare 3.97 and Simulator

> Deep reference for [AGENTS.md](../../AGENTS.md). This fork targets the
> **Waveshare ESP32-S3-ePaper-3.97** (800×480 SSD1677) as its sole hardware
> image, plus the desktop **X4-class simulator** for host-side development.

## TL;DR

```bash
pio run                        # default env: waveshare_epaper_397
pio run -t upload              # flash the board on USB
pio run -e waveshare_epaper_397_nightly   # Nightly packaging env (CI)
pio run -e simulator -t run_simulator      # desktop simulator (SDL2)
```

Nightly firmware is published only for `waveshare_epaper_397`. See
[firmware-release.md](firmware-release.md) and
[waveshare-epaper-397.md](waveshare-epaper-397.md) for packaging, flashing, and
acceptance notes.

## Hardware profile

| Item | Detail |
|---|---|
| SoC | ESP32-S3 (16 MB flash, OPI PSRAM) |
| Display | 3.97" 800×480 SSD1677 |
| Storage | 4-bit SDMMC |
| Input | Four physical buttons; optional BLE page-turner host (PSRAM allocator) |
| USB | USB Serial/JTAG default; MSC for file transfer activity |
| Audio | ES8311 + NS4150B — physical button feedback only (`sound_feedback_hardware`) |

The PlatformIO profile is `waveshare_epaper_397_hardware` in
[`platformio.ini`](../../platformio.ini). It extends `sound_feedback_hardware`,
inherits `s3_ble_psram` (NimBLE host in PSRAM, prebuilt `dio_opi` core for
TinyUSB), and embeds the Waveshare board tag in the firmware image. Manual
flashes must use a build for this board; the packager rejects mismatched board
tags.

## Simulator

`env:simulator` is a native build using the pinned
[crosspoint-simulator](https://github.com/0x1abin/crosspoint-simulator) fork.
CrossMux configures it for **Waveshare ESP32-S3 ePaper 3.97** (800×480, no touch)
using the same vendor `FunctionButtonGesture` header as hardware
(`lib/waveshare397_input/include/`). SDL keys map to the four **face buttons**
(GPIO0 BOOT, GPIO5 Function, GPIO4 Left, GPIO6 Right); `FunctionButtonGesture`
then produces logical Back/Confirm/Left/Right/Up/Down like
`InputManager::updateFunctionMultiGesture` on the device.

Install SDL2 and OpenSSL development headers, place EPUBs under `fs_/books/`, then:

```bash
pio run -e simulator -t run_simulator
```

| SDL key | Face button | GPIO | Notes |
|---|---|---|---|
| Escape | BOOT | 0 | Short press alone → no logical action; hold + Left/Right → Up/Down |
| Return / Enter | Function | 5 | Click → Confirm (~300 ms window); double-click → Back |
| Left arrow | Left | 4 | Logical Left/Right on release; chords with BOOT per gesture rules |
| Right arrow | Right | 6 | Same as Left |
| P | Side power (AXP2101) | — | Outside face-button map; sleep shortcut **S** unchanged |

Up/Down arrow keys are not mapped to face buttons. Logical Up/Down come from
**BOOT + Left/Right** through `FunctionButtonGesture`, not from keyboard Up/Down.

Hardware CI and main CI keep `simulator` buildable alongside
`waveshare_epaper_397_nightly`.

## BLE and memory

Waveshare uses the shared S3 BLE host profile (`s3_ble_psram`): controller on
the prebuilt core, host heap primarily in PSRAM, IPC stack sizing adapter, and
the same reader/settings memory gates as other S3 targets. Successful firmware
builds do not replace on-device acceptance for pairing, reconnect, and sleep
recovery.

## Related docs

- [waveshare-epaper-397.md](waveshare-epaper-397.md) — bring-up, flashing, recovery
- [build-system.md](build-system.md) — PlatformIO environments and flags
- [firmware-release.md](firmware-release.md) — Nightly channel and OTA indexes
