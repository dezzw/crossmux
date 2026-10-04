# CrossMux (Waveshare)

**English** | [简体中文](./README.zh-CN.md)

**CrossMux** is a community fork of [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader), tuned for the **Waveshare ESP32-S3 ePaper 3.97** (800×480, 16 MiB flash, 8 MiB PSRAM). Reading comes first, with lightweight apps, reading analytics, standby faces, and on-demand services alongside the reader.

[Releases](https://github.com/0x1abin/crossmux/releases) · [User guide](./USER_GUIDE.md) · [Contributing](./docs/contributing/README.md)

![CrossMux on e-ink hardware](./docs/images/cover.jpg)

## Features

- **Reading and library**: EPUB, TXT, XTC/XTCH and images; chapter navigation, bookmarks, dictionaries, custom fonts, reading backgrounds, and KOReader progress sync.
- **Wireless workflows**: browser file transfer and settings, Calibre wireless, OPDS downloads, WebDAV, and device OTA updates.
- **Apps**: file transfer, OPDS browser, reading analytics, standby faces, and (China profile) WeRead. [Apps guide](./src/activities/apps/README.md).
- **WeRead**: QR login, bookshelf browsing, EPUB downloads for offline reading, and progress sync. Available in the China content profile. [WeRead guide (Chinese)](./src/activities/apps/weread/README.md).
- **Reading analytics and standby**: reading statistics, heatmaps, profiles and achievements; clock and Chinese almanac faces. [Analytics guide](./src/activities/apps/reading-stats/README.md).
- **Languages and development**: 33 UI languages in one firmware image, plus a desktop simulator for UI development.

> **WeRead security:** this unofficial Web protocol may change. Device traffic is encrypted, but its client does not verify the server certificate or hostname; use it only on a trusted network. The native simulator verifies certificates through the host trust store. See the [transport details](./docs/engineering/chinese-build.md#weread-transport).

## Device and releases

| Item | Detail |
|---|---|
| Hardware | [Waveshare ESP32-S3 ePaper 3.97](https://www.waveshare.com/) |
| Chip | ESP32-S3 (16 MiB flash, 8 MiB OPI PSRAM) |
| Panel | 800×480 SSD1677 |
| Channels | Nightly (see [releases](https://github.com/0x1abin/crossmux/releases)) |

Build environment and version: [platformio.ini](./platformio.ini). Packaging and OTA: [firmware-release.md](./docs/engineering/firmware-release.md). Hardware notes and acceptance: [waveshare-epaper-397.md](./docs/engineering/waveshare-epaper-397.md).

## Install firmware

1. Open [CrossMux Releases](https://github.com/0x1abin/crossmux/releases), pick the **Waveshare ePaper 3.97** Nightly asset, and follow that release’s install notes.
2. Back up SD card data before changing firmware. Use the full install package for first flash; application-only `firmware.bin` is not a complete first-install image.
3. Flash and recovery commands (offsets, USB port) are board-specific—follow [waveshare-epaper-397.md](./docs/engineering/waveshare-epaper-397.md), not generic ESP32-C3/Xteink instructions.

Installed firmware can update over Wi-Fi when a matching package is published on the release channel.

## Chinese fonts and content profiles

One language-unified firmware per build. Simplified Chinese selects the China content profile for regional metadata and apps such as WeRead; OTA, font downloads, and other network services always use `crossmux.com` with the global OTA variant. Other UI languages use the Global content profile.

The UI includes compact 8/10/12pt Simplified-Chinese fallback fonts. Built-in reader font choices share a 12pt offline fallback; complete families, other sizes, style variants, and broader Unicode coverage use SD-card `.cpfont` files. Embedded fonts are a subset, so rare or Traditional Chinese characters may require an appropriate SD font.

Download fonts from **Settings > Reader > Manage Fonts**, or copy converted fonts to the SD card. See [SD-card fonts](./docs/sd-card-fonts.md) and [Chinese support](./docs/engineering/chinese-build.md).

## Development quick start

Install PlatformIO Core (`pio`) and Python 3; the repository pins its pioarduino platform. Full code checks also need clang-format 21+, CMake, and Ninja. See [Getting Started](./docs/contributing/getting-started.md).

```bash
git clone --recursive https://github.com/0x1abin/crossmux.git
cd crossmux

# If submodules were not initialized:
git submodule update --init --recursive

# Waveshare ESP32-S3 ePaper 3.97 (default env)
pio run

# Build and flash to a connected board
pio run -t upload
```

The application binary is `.pio/build/waveshare_epaper_397/firmware.bin`. See [build-system.md](./docs/engineering/build-system.md) and [waveshare-epaper-397.md](./docs/engineering/waveshare-epaper-397.md).

### Desktop simulator

Install SDL2 and curl (plus OpenSSL development headers on Linux), place EPUBs in `fs_/books/`, then run:

```bash
pio run -e simulator -t run_simulator
```

The [Waveshare-profile simulator fork](https://github.com/dezzw/crosspoint-simulator) is pinned in `platformio.ini` (face-button input is implemented in that repo, not CrossMux overlays). It previews UI and button input; it does not validate e-ink waveforms, power consumption, or Waveshare PMIC timing.

### Checks and debugging

```bash
./bin/ci-check       # Full code-change checks; does not rewrite sources
pio device monitor  # Serial logs from a connected device
```

Use [Testing and Debugging](./docs/contributing/testing-debugging.md) for focused checks and the enhanced serial monitor. Documentation-only changes need link, command, and whitespace checks rather than firmware builds.

## Documentation and contributing

- [User guide](./USER_GUIDE.md) · [Web transfer](./docs/webserver.md) · [Web API](./docs/webserver-endpoints.md)
- [Project scope](./SCOPE.md) · [Governance](./GOVERNANCE.md) · [Contributor guide](./docs/contributing/README.md)
- [Agent instructions](./AGENTS.md) · [Engineering reference](./docs/engineering/index.md) · [Buttons and UI](./docs/contributing/touch-and-ui.md)
- [Cache management](./docs/engineering/cache-management.md) · [File formats](./docs/file-formats.md)

Report bugs and propose changes in [CrossMux Issues](https://github.com/0x1abin/crossmux/issues). Contributions target **`0x1abin/crossmux:main`**; keep each PR focused and describe its verification. Existing CrossPoint class names and the `/.crosspoint` SD data directory remain compatibility details. That directory also holds settings and progress; do not delete it merely to clear a book cache.

## Credits

Thanks to [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader), [Inx](https://github.com/obijuankenobiii/inx), [cpr-vcodex](https://github.com/franssjz/cpr-vcodex), and their contributors, and to [diy-esp32-epub-reader](https://github.com/atomic14/diy-esp32-epub-reader) for the original inspiration.

CrossMux is not affiliated with Waveshare or any device manufacturer. See [LICENSE](./LICENSE) for the repository license.
