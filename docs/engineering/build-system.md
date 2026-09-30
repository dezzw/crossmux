# Build System & Build Flags

> Deep reference for [AGENTS.md](../../AGENTS.md). Covers PlatformIO usage, the
> build environments, the critical build flags that change firmware behavior, and
> personal local overrides.

## Build System: PlatformIO

**PlatformIO is BOTH a VS Code extension AND a CLI tool**:

1. **VS Code Extension** (Recommended):
   * Extension ID: `platformio.platformio-ide` (see `.vscode/extensions.json`)
   * Provides: Toolbar buttons, IntelliSense, integrated build/upload/monitor
   * Configuration: `.vscode/c_cpp_properties.json`, `.vscode/tasks.json`
   * Usage: Click Build (✓), Upload (→), or Monitor (🔌) buttons

2. **CLI Tool** (`pio` command):
   * **Installation**: Python package (typically `pip install platformio`)
   * **Windows Location**: `C:\Users\<user>\AppData\Local\Programs\Python\Python3xx\Scripts\pio.exe`
   * **Verify**: `which pio` (Git Bash) or `where.exe pio` (cmd)
   * **Usage**: `pio run`, `pio run -t upload`, etc.

**Configuration Files**:
* `platformio.ini`: Main build configuration (committed to git)
* `platformio.local.ini`: Local overrides (gitignored, create if needed)
* `partitions.csv`: ESP32 flash partition layout

## Build Environment
* **Standard**: C++20 (`-std=c++2a`). No Exceptions, No RTTI.
* **Logging**: ALWAYS use `LOG_INF`, `LOG_DBG`, or `LOG_ERR` from `Logging.h`. Raw Serial output is deprecated.
* **Environments** (in `platformio.ini`):
  * `waveshare_epaper_397` (**default**): Waveshare ESP32-S3 ePaper 3.97 development build (LOG_LEVEL=2, serial enabled)
  * `waveshare_epaper_397_nightly`: Nightly packaging build (LOG_LEVEL=1, RC version string from `CROSSPOINT_RC_HASH`)
  * `simulator`: Native X4-class desktop simulator (pinned simulator fork)

Shared ini sections used by the Waveshare profile include `base`, `ble_host`,
`s3_ble`, `s3_ble_psram`, `s3_nightly`, and `sound_feedback_hardware`.

`bin/ci-check` builds `waveshare_epaper_397` after cppcheck. Pull-request CI runs
`pio check`, builds `waveshare_epaper_397_nightly`, and runs host unit tests.
Path-filtered Hardware CI builds `simulator` and
`waveshare_epaper_397_nightly` when hardware-sensitive files change.

Bluetooth Page Turner Beta is compiled into the Waveshare hardware profile.
The runtime Bluetooth switch defaults to off; the simulator uses SDK stubs.
Waveshare inherits PSRAM NimBLE host settings through `s3_ble_psram` and keeps
the prebuilt `dio_opi` core so the TinyUSB MSC component graph remains intact.

The SDK's obsolete passkey callback is removed only from a generated source copy
under `$BUILD_DIR/ble-compat`; the SDK and NimBLE dependency sources are never
rewritten. The source is a build dependency and unexpected callback signatures
fail the build. The same translation unit includes the `_btLibraryInUse` weak
shim for both the custom-core bootstrap (which omits application sources) and
the final firmware, without suppressing NimBLE's Arduino BT usage header.

The pinned prebuilt S3 core still creates 1 KiB IPC stacks. BLE controller
interrupt allocation can overflow `ipc0`; upstream Arduino lib-builder #386
raises the budget to 2 KiB. S3 BLE builds use a narrow link adapter at task
creation to apply that minimum only to `ipc0`/`ipc1` on their matching cores.
It adds at most 2 KiB of internal stack RAM across both tasks and preserves
larger configured stacks, allocation failures, other tasks, and the prebuilt
TinyUSB core. The adapter travels with the same bootstrap-compatible source;
remove it when the pinned core supplies the upstream budget. BLE diagnostics
include both IPC stack high-water marks; check them after repeated starts.

When switching from a custom core to a prebuilt target, retain the framework's
`sdkconfig.orig` marker until PlatformIO restores the original core package.
Restoring only `sdkconfig` leaves custom IDF archives behind; mixing these with
an untouched `dio_opi` header can omit PSRAM initialization entirely.
`scripts/tests/test_pioarduino_cache.py` covers this transition.

For isolated cache-switch validation, run builds sequentially with all four
overrides below (the directories are gitignored). Do not copy compiled core
packages from an existing PlatformIO installation into this environment.

```bash
export PLATFORMIO_CORE_DIR="$PWD/.platformio/ble-psram"
export PLATFORMIO_BUILD_DIR="$PWD/.pio/ble-psram-build"
export PLATFORMIO_BUILD_CACHE_DIR="$PWD/.cache/ble-psram"
export IDF_COMPONENT_CACHE_PATH="$PWD/.cache/ble-psram-idf-components"
pio run -e waveshare_epaper_397_nightly
```

Use the same overrides when uploading. Check the resulting ELF for the actual
PSRAM initialization and heap-registration call paths, not just the
`BOARD_HAS_PSRAM` macro or `psramInit` symbol. Runtime BLE diagnostics must report
nonzero PSRAM capacity and a successful allocator probe before connection tests.

## Desktop Simulator

Install SDL2 and `curl` (plus OpenSSL development headers on Linux), place EPUB
files under `fs_/books/`, and run:

```bash
pio run -e simulator -t run_simulator
```

The simulator implementation and launcher come from the pinned
[`0x1abin/crosspoint-simulator`](https://github.com/0x1abin/crosspoint-simulator)
fork; the exact revision is recorded in `platformio.ini`.
Arrow keys are Up/Down, `P` is Power, mouse input provides touch, and `S` sleeps.
The simulator covers UI, input, RTC state, and sleep/wake flows. It does not
emulate EPD waveforms, SDMMC contention, PSRAM, or power consumption.

## Critical Build Flags
These flags in `platformio.ini` fundamentally affect firmware behavior:

```cpp
-DEINK_DISPLAY_SINGLE_BUFFER_MODE=1  // Single framebuffer (saves 48KB RAM!)
-DARDUINO_USB_MODE=1                 // Enable USB CDC
-DARDUINO_USB_CDC_ON_BOOT=1          // Serial available immediately at boot
-DXML_CONTEXT_BYTES=1024             // XML parser memory limit (EPUB parsing)
-DUSE_UTF8_LONG_NAMES=1              // SD card long filename support
-DXML_GE=0                           // Disable XML general entities (security)
-DDESTRUCTOR_CLOSES_FILE=1           // FsFile destructor auto-closes (SdFat)
```

**DESTRUCTOR_CLOSES_FILE implications**:
- SdFat's `FsBaseFile` destructor calls `close()` automatically when the object goes out of scope
- **Do NOT add explicit `file.close()` calls** for local `FsFile` variables — the destructor handles it
- Explicit `close()` is still required when closing before delete/reopen or in `onExit()` for member `FsFile` variables

**SINGLE_BUFFER_MODE implications**:
- Only ONE framebuffer exists (not double-buffered)
- Grayscale rendering requires temporary buffer allocation (`renderer.storeBwBuffer()`)
- Must call `renderer.restoreBwBuffer()` to free temporary buffers

See [device-variants.md](device-variants.md) for Waveshare-specific capabilities
(USB MSC, sound feedback, board tag).

---

## Local Development Configuration

### platformio.local.ini (Personal Overrides)

**Purpose**: Personal development settings that should NEVER be committed.

**Example** `platformio.local.ini`:
```ini
# platformio.local.ini (gitignored)
[env:waveshare_epaper_397]
upload_port = COM7
monitor_port = COM7

build_flags =
  ${waveshare_epaper_397_hardware.build_flags}
  -DMY_DEBUG_FLAG=1
```

**Rules**:
- **NEVER commit** `platformio.local.ini`
- **NEVER put** personal info (serial ports, credentials) in main `platformio.ini`

See also: [getting-started](../contributing/getting-started.md), [testing-and-debugging.md](testing-and-debugging.md).
