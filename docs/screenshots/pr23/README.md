# PR #23 font specimens

800×480 reader-style specimens at Waveshare / simulator geometry, rasterized with
the same pinned masters, point sizes, and 150 DPI settings as `fontconvert_sdcard.py`.
Each PNG includes the mixed punctuation line `“”—…€℃№①②` plus CJK/Latin samples
with regular, **bold**, and italic masters where applicable.

Settings vs legacy crossmux.com m1-b4 and upstream Literata/Inter: see
[`font-settings-comparison.md`](font-settings-comparison.md).

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
