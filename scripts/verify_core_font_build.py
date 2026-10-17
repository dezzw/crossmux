#!/usr/bin/env python3
"""Verify core SD font glyph coverage and .cpfont size limits after build."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCRIPTS = ROOT / "scripts"
OUTPUT = ROOT / "lib/EpdFont/scripts/output"
MAX_FONT_BYTES = 25 * 1024 * 1024
EXPECTED_LATIN_GLYPHS = 1850

sys.path.insert(0, str(SCRIPTS))
from cpfont_codepoints import cpfont_codepoints, cpfont_glyph_stats  # noqa: E402
from legacy_noto_m1_b4 import download_legacy_noto  # noqa: E402


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--legacy-noto", type=Path, help="Cached legacy NotoSansSC_12.cpfont")
    args = parser.parse_args()

    errors: list[str] = []

    sys.path.insert(0, str(SCRIPTS))
    from cpfont_reader import load_cpfont_style  # noqa: E402

    for family in ("Literata", "Inter"):
        path = OUTPUT / family / f"{family}_12.cpfont"
        if not path.is_file():
            errors.append(f"missing {path}")
            continue
        _styles, glyphs, size = cpfont_glyph_stats(path)
        if glyphs != EXPECTED_LATIN_GLYPHS:
            errors.append(f"{family}_12: expected {EXPECTED_LATIN_GLYPHS} glyphs/style, got {glyphs}")

    lit_bold = OUTPUT / "Literata/Literata_12.cpfont"
    if lit_bold.is_file():
        bold = load_cpfont_style(lit_bold, 1)
        ga = bold.get_glyph(ord("a"))
        gA = bold.get_glyph(ord("A"))

        def _ink(g) -> int:
            from cpfont_reader import _coverage_2bit, coverage_draws_bw  # noqa: WPS433

            if not g or not g.bitmap:
                return 0
            return sum(
                1
                for y in range(g.height)
                for x in range(g.width)
                if coverage_draws_bw(_coverage_2bit(g.bitmap, y * g.width + x))
            )

        if not ga or not gA:
            errors.append("Literata bold: missing 'a' or 'A' glyph")
        elif ga.bitmap == gA.bitmap:
            errors.append("Literata bold: lowercase 'a' must differ from 'A' in .cpfont")
        elif ga.width < 4 or gA.width < 4 or _ink(ga) < 8 or _ink(gA) < 8:
            errors.append("Literata bold: 'a'/'A' bitmaps look empty or corrupt")

    noto_dir = OUTPUT / "NotoSansSC"
    if noto_dir.is_dir():
        for cpfont in sorted(noto_dir.glob("NotoSansSC_*.cpfont")):
            styles, _glyphs, size = cpfont_glyph_stats(cpfont)
            if size > MAX_FONT_BYTES:
                errors.append(f"{cpfont.name}: {size} bytes exceeds {MAX_FONT_BYTES} limit")
            if styles != 2:
                errors.append(f"{cpfont.name}: expected 2 styles (regular+bold), got {styles}")
    else:
        errors.append(f"missing {noto_dir}")

    legacy_path = args.legacy_noto or (ROOT / "lib/EpdFont/scripts/.cache/NotoSansSC_12.cpfont")
    try:
        download_legacy_noto(legacy_path)
        old_bitmap = set(cpfont_codepoints(legacy_path, require_bitmap=True))
        new_path = OUTPUT / "NotoSansSC/NotoSansSC_12.cpfont"
        if new_path.is_file():
            new_bitmap = set(cpfont_codepoints(new_path, require_bitmap=True))
            missing = sorted(old_bitmap - new_bitmap)
            if missing:
                errors.append(
                    f"NotoSansSC_12 missing {len(missing)} legacy bitmap codepoints "
                    f"(first: {', '.join(hex(c) for c in missing[:8])})"
                )
        else:
            errors.append(f"missing {new_path}")
    except OSError as exc:
        errors.append(f"legacy NotoSansSC download/compare failed: {exc}")

    if errors:
        for err in errors:
            print(f"ERROR: {err}", file=sys.stderr)
        return 1

    print(json.dumps({"status": "ok", "latinGlyphsPerStyle": EXPECTED_LATIN_GLYPHS}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
