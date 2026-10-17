# PR #23 font build settings comparison

| Setting | Legacy m1-b4 NotoSansSC | PR #23 |
| --- | --- | --- |
| Styles in manifest | `regular` only | **regular + bold** (2 styles / file) |
| Bold master | duplicate Regular | **NotoSansSC-Bold.otf** (pinned noto-cjk) |
| Raster | 2-bit, 150 DPI, native hinting | same |
| Latin / symbols | OST-style intervals | `latin-ext,punctuation,cjk,greek,cyrillic,ipa-chars,combining,letterlike,enclosed-alphanumerics,ext-a,cjk-symbols,symbols` |
| Hanzi / Ext-A / Ext-B | legacy subset OTF | `noto_m1_b4_hanzi_coverage.txt` (derived from legacy m1-b4 bitmap coverage) |
| Literata / Inter intervals | upstream `latin-ext,greek,cyrillic,ipa-chars` | same; **latin-ext** now includes `U+20A0–20C0`, `U+2100–214F`, `U+2212` |
| Literata / Inter glyphs @ 12pt | 1850 / style | **1850** / style (verified in CI) |
| Noto legacy bitmap superset | — | **`old − new = ∅`** vs crossmux.com m1-b4 @ 12pt |
| Max `.cpfont` size | — | **≤ 25 MiB** (`FontManifestCatalog.h`) |

Regenerate metrics: `python3 lib/EpdFont/scripts/build-sd-fonts.py && python3 scripts/verify_core_font_build.py`
