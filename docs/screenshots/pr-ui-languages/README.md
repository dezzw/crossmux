# PR UI language screenshots

Captured from the Waveshare-profile desktop simulator (`pio run -e simulator`) using `CROSSPOINT_SIM_INPUT_SCRIPT` / `CROSSPOINT_SIM_SCREENSHOTS` with `xvfb-run`, then converted BMP → PNG with ImageMagick. Before shots used `main` (all YAML languages); after shots used this branch (`i18n_languages = ZH_CN`).

Keyboard shots use `keyboardLayouts: 2` (French AZERTY only) on the OPDS Add Server text field so the on-screen layout is visibly non-QWERTY while UI strings stay EN or ZH_CN. Re-capture with `scripts/capture_pr_ui_languages_keyboard_azerty.sh <before|after> <EN|ZH_CN>`.
