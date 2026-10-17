# PR #23 font specimens

800×480 1-bit renders using the same pinned masters and point sizes as the SD
font build (`fontconvert_sdcard.py` at 150 DPI). They mirror the Waveshare /
simulator viewport geometry for visual review before flashing release assets.

| File | Content |
|------|---------|
| `notosans_sc_*.png` | Simplified Chinese regular, **Bold master**, and italic (Regular master) |
| `literata_*.png` | English serif regular, bold, italic |
| `inter_*.png` | English sans regular, bold, italic (static instances from pinned Inter VF) |

Regenerate after font source changes:

```bash
python3 scripts/fetch_notosans_sc_source.py
python3 lib/EpdFont/scripts/build-sd-fonts.py --only Literata,Inter
python3 scripts/generate_pr23_font_screenshots.py
```
