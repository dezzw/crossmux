# Hardware Constraints & The Resource Protocol

> Deep reference for [AGENTS.md](../../AGENTS.md). This fork targets the
> **Waveshare ESP32-S3 ePaper 3.97**: dual-core ESP32-S3 @ 240 MHz, **16 MiB
> flash**, **8 MiB OPI PSRAM**, and **800×480** SSD1677 e-ink. Budget shared
> reader code for **~512 KiB internal SRAM** first; PSRAM is for large, explicit
> buffers (framebuffer, audio), not a substitute for tight internal-heap discipline.

Board-specific wiring, buttons, and acceptance notes:
[waveshare-epaper-397.md](waveshare-epaper-397.md).

## Platform snapshot

| Resource | Waveshare baseline |
|---|---|
| Internal SRAM | ~512 KiB usable for heap, stacks, DMA-capable allocations |
| PSRAM | 8 MiB OPI; primary 48 KiB monochrome framebuffer when enabled |
| Framebuffer | 800 × 480 ÷ 8 = **48 000 bytes** (single buffer in normal builds) |
| Display | Monochrome e-ink; full refresh ~1–2 s; partial/grayscale paths for reading |
| Storage | SD card (4-bit SDMMC); aggressive EPUB/cache on card |

**PSRAM vs internal:** Code and small hot structures stay in internal RAM.
Display scratch, large transient decode buffers, and similar assets may use
PSRAM only when the build and driver contract allow it. Anything that must be
DMA-safe or latency-critical still belongs in internal heap unless the HAL
documents otherwise.

## The Resource Protocol

Same spirit as the upstream CrossMux guide, applied to this S3 target:

1. **Stack safety:** Keep large locals off the stack; prefer static pools or
   activity-owned buffers allocated in `onEnter()`.
2. **Heap fragmentation:** No allocate/free per frame or per page turn. Reuse
   buffers for the activity lifetime; `.reserve()` before `push_back()` loops.
3. **Flash for constants:** UI string tables and lookup data stay `static const`
   / `constexpr` in flash, not copied into DRAM at runtime.
4. **Hot-path strings:** Avoid `std::string` / Arduino `String` in render and
   reader loops; use `string_view`, fixed `char[]`, and `snprintf`.
5. **UI text:** User-facing strings use `tr()`; logs may be hardcoded.
6. **`constexpr` first:** Tables and sizes known at compile time should be
   `constexpr` for flash placement and dead-code elimination.
7. **SPIFFS / settings writes:** Write only on change; debounce progress and
   frequent toggles to protect flash wear.
8. **Allocation failure:** With `-fno-exceptions`, bare `new` aborts on failure.
   Use `makeUniqueNoThrow<T>()` from `lib/Memory/Memory.h` (or
   `new (std::nothrow)` when a C API takes ownership); null-check and `LOG_ERR`.

See also: [memory-and-allocation.md](memory-and-allocation.md),
[esp32-pitfalls.md](esp32-pitfalls.md) (alignment, ISRs, RISC-V/S3 shared rules).

## Verification

After changes that affect memory or startup:

- Log free heap and minimum free heap after boot and after opening a large EPUB
  (`pio device monitor` or `python3 scripts/debugging_monitor.py`).
- On hardware, exercise sleep/wake, SD mount, and several page turns without
  monotonic heap decline.
