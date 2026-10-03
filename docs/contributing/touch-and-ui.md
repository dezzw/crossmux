# Buttons, BLE-HID, and UI Development

This fork builds for the **Waveshare ESP32-S3 ePaper 3.97** only. Input is
**four physical face buttons** (Back, Left, Function, Right) plus **PMIC Power**,
optional **BLE HID** remotes in Controls settings, and **no capacitive touch**.
There is no touch controller and no on-screen tap targets for primary navigation.

All new screens should use **FreeInkUI** through the firmware hosts below.
Physical buttons and BLE-HID actions share the same interaction table; do not
hand-roll hit rectangles or read GPIO outside the HAL.

## Host choice

| Screen shape | Base class | Reference |
|---|---|---|
| Single list | `UiListActivity` | `LanguageSelectActivity`, `RecentBooksActivity` |
| Tabbed lists | `UiTabListActivity` | `SettingsActivity` |
| Custom layout | `UiAppHost` | `WifiSelectionActivity`, `EpubReaderPercentSelectionActivity` |
| Modal picker | `OptionPopup` / `ConfirmationActivity` | `OtaUpdateActivity` |

Legacy bridge helpers (`rowTouch`, `wasTapInRect`, manual rect hit tests) exist
only for old themed surfaces. **Do not use them in new code.**

## Spacing and hit geometry (6 px rule)

Adjacent interactive controls need at least **6 px of visible background**
between them in every theme and orientation ([AGENTS.md](../../AGENTS.md) rule 14).
Hit regions must stay inside the drawn control and must not overlap a neighbour.

Calculate each rectangle once; use the same geometry for drawing and for
FreeInkUI interaction registration:

```cpp
const Rect action = actionRect(index);
drawAction(action, label);
frame.hit(action, ACTION_SELECT, index, fui::InputTouch);
```

Do not shrink visible gaps by enlarging hit boxes into the space between controls.

On this hardware, “hit testing” applies to **focus rings, list rows, and
FreeInkUI controls** driven by buttons or BLE-HID—not finger coordinates.

## Physical buttons and BLE-HID

- Route input through `MappedInputManager::Button::*` logical buttons, never raw
  `HalGPIO::BTN_*` except in button-remap activities.
- One physical gesture → one action: gate inherited held buttons across popups
  and activities; consume the release that triggered a transition.
- BLE-HID mapping lives under **Settings → Controls → Bluetooth**; same action
  set as face buttons (page turn, Confirm, Back, directions). Do not assume BLE
  is always on—memory-heavy work may tear it down temporarily.

Button timing and chord behavior for this board (Function double-click as
Back, BOOT+dial as Up/Down) are documented in
[waveshare-epaper-397.md](../engineering/waveshare-epaper-397.md).

## FUI screen rules (short)

- Register handlers with `app.on(...)` in `onEnter()`; call `app.clearTapFlash()`
  before leaving a screen when applicable.
- Render through `GUI` / `UITheme`; use `renderer.getScreenWidth()` /
  `getScreenHeight()`—never hardcode 800×480 in activity code.
- Keep steady-state render paths allocation-free: row vectors owned by the
  activity, rebuilt when data changes—not inside every `buildScreen()`.

Further UI/input invariants: [ui-and-input.md](../engineering/ui-and-input.md),
[architecture-and-patterns.md](../engineering/architecture-and-patterns.md).

## Build and test

Default environment:

```bash
pio run -e waveshare_epaper_397
pio run -e simulator -t run_simulator   # SDL2 desktop preview; buttons only
```

The simulator does not replace hardware checks for power, e-ink timing, or audio
feedback on the Waveshare board.
