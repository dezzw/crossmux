# PR #23 font build settings comparison

Reference baselines:

- **Legacy NotoSansSC (crossmux.com m1-b4):** manifest at `https://crossmux.com/api/assets/fonts/m1-b4/fonts.json`, release prefix `71141b4aeb4a…`
- **CrossPoint / CrossMux Latin families:** `0x1abin/crossmux` `lib/EpdFont/scripts/sd-fonts.yaml` on `main` (Literata / Inter URLs and intervals)
- **PR #23 build:** `lib/EpdFont/scripts/sd-fonts.yaml` + `font-release-pins.yaml` on branch `cursor/self-host-core-fonts-pin-ota-f2de`

## Raster / container

| Setting | Legacy m1-b4 NotoSansSC | PR #23 (all families) | Upstream sd-fonts Literata/Inter |
| --- | --- | --- | --- |
| Tool | `fontconvert_sdcard.py` (CrossMux OST) | Same script + `build-sd-fonts.py` | Same |
| `.cpfont` version | 4 | 4 | 4 |
| Bitmap depth | 2-bit greyscale (`flags=1`) | 2-bit greyscale | 2-bit greyscale |
| DPI / ppem | 150 DPI (`size pt × 150/72`) | 150 DPI | 150 DPI |
| Default hinting | Native (no `--force-autohint`) | Native | Native |
| `force_autohint` in YAML | not set | not set | not set |

## NotoSansSC coverage

| Setting | Legacy m1-b4 (measured) | PR #23 |
| --- | --- | --- |
| Manifest `styles` | `["regular"]` only | Built: **regular + bold** (2 styles per `.cpfont`) |
| Bold source | Synthesized / duplicate Regular in early PR23 | **NotoSansSC-Bold.otf** (pinned noto-cjk SubsetOTF) |
| Italic | N/A (single style) | No italic slot; device maps italic → regular |
| Hanzi selection | ~**9 680** glyphs @ 12pt (1 style), interval-backed | **GB 2312 allowlist** (`chinese_gb2312_hanzi.txt`, 6763 ideographs) + interval presets |
| Interval presets | Inferred match: Latin/Greek/Cyrillic + CJK blocks + symbols (OST recipe) | `ascii,latin1,punctuation,greek,cyrillic,ipa-chars,symbols,cjk,cjk-symbols` |
| Hanzi filter | Subset OTF + interval scan (legacy pipeline) | `--codepoints-file` filters `3400–4DBF`, `4E00–9FFF`, `F900–FAFF` |
| Glyphs @ 12pt | 9680 × 1 style | 9261 × 2 styles |
| Bytes @ 12pt | 1 533 978 | 2 939 105 (~1.92× legacy; bold + slightly different subset) |

Legacy is **not** the firmware’s 3500-character `cn_common_chars.txt` built-in tier; the published SD `NotoSansSC` set is closer to **GB 2312-scale** hanzi plus Latin/Greek/Cyrillic/symbol intervals. PR #23 targets that recipe explicitly; ~**419** fewer hanzi slots than legacy @ 12pt (legacy includes additional ideographs outside strict GB 2312, including some Extension B entries present in the old subset OTF).

## Literata / Inter

| Setting | Upstream `main` sd-fonts | PR #23 |
| --- | --- | --- |
| Literata masters | Separate Regular/Bold/Italic/BoldItalic TTF @ literata `0c2761b…` | Same URLs (pinned in `font-release-pins.yaml`) |
| Inter masters | Variable font + `variable: {wght, opsz}` per style | Same; **fonttools instancer** → static TTFs in `instanced_fonts/Inter/` |
| Inter bold/italic | Real axis instances (not duplicated Regular) | Same |
| Intervals | `latin-ext,greek,cyrillic,ipa-chars` | Same |
| Sizes in manifest | 12, 14, 16, 18 | Same |

## Release asset sizes (bytes)

Run after a local font build:

```bash
python3 lib/EpdFont/scripts/build-sd-fonts.py
python3 scripts/report_pr23_font_metrics.py
```
