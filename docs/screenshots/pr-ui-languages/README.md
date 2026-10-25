# PR UI language screenshots

Captured from the Waveshare-profile desktop simulator (`pio run -e simulator`) using `CROSSPOINT_SIM_INPUT_SCRIPT` / `CROSSPOINT_SIM_SCREENSHOTS` with `xvfb-run`, then converted BMP → PNG with ImageMagick. Before shots used `main` (all YAML languages); after shots used this branch (`i18n_languages = ZH_CN`).

**AZERTY keyboard shots:** use sim SD `settings.json` with `"keyboardLayouts":2` (French AZERTY only) plus `"language":"EN"` or `"ZH_CN"` and matching `"contentProfile"` (0 for EN, 1 for ZH_CN), sample EPUB under `Books/`, then after `pio run -e simulator`:

```bash
CROSSPOINT_SIM_SD=<sim-sd-dir> CROSSPOINT_SIM_INPUT_SCRIPT='2000:BOOT;2400:RIGHT;2700:RIGHT;3000:RIGHT;3300:FUNCTION;3700:DOWN;4000:DOWN;4300:DOWN;4600:FUNCTION;4900:DOWN;5200:DOWN;5500:DOWN;5800:DOWN;6100:DOWN;6400:DOWN;6700:DOWN;7000:DOWN;7300:DOWN;7600:DOWN;7900:FUNCTION;8300:ENTER;8800:ENTER;12000:QUIT' CROSSPOINT_SIM_SCREENSHOTS='9500:docs/screenshots/pr-ui-languages/<before|after>/<EN|ZH_CN>/<prefix>-keyboard.bmp' xvfb-run -a .pio/build/simulator/program
```

Convert the BMP to PNG (e.g. ImageMagick `convert`). The shot at 9500 ms is OPDS Add Server → Server Name with AZERTY visible.

**Keyboard layout list** (`*-keyboard-layouts.png`): same simulator setup with `"keyboardLayouts":511`, navigate Settings → System → Keyboard Layouts (`9400:FUNCTION` after fifteen `DOWN` steps from the expanded System list; screenshot at `10000` ms). Native endonym labels should match `main` (Cyrillic, Hebrew, accents) in both EN and ZH_CN UI shells.
