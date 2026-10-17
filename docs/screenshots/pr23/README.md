# PR #23 font specimens

800×480 **1-bit** images rendered from built **`.cpfont`** files via
`scripts/render_cpfont_specimen.py` (baseline cursor, fp4 advances, kerning,
2-bit threshold, word wrap). This matches the reader’s `GfxRenderer` layout
more closely than raw FreeType.

| File | Content |
|------|---------|
| `notosans_sc_*.png` | NotoSansSC regular + **bold** @ 12/14/16/18pt |
| `literata_*.png` | Literata regular, bold, italic @ 12/14/16pt |
| `inter_*.png` | Inter regular, bold, italic @ 12/14/16pt |

Each block includes an English or Chinese body paragraph and the mixed line
`“”—…€℃№①②` plus CJK/Latin.

**Symbol coverage (Literata/Inter @ 12pt):** `€` and `℃` are present in the
`.cpfont`; **①②** are not in the pinned TTFs (interval slots omitted after
font validation). On device, missing codepoints use the **missing-glyph**
placeholder (outline box), not a fallback font, unless the string triggers
CJK routing to another family.

Regenerate after font build:

```bash
python3 lib/EpdFont/scripts/build-sd-fonts.py
python3 scripts/render_cpfont_specimen.py
```

Settings comparison: [`font-settings-comparison.md`](font-settings-comparison.md).
