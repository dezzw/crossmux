#!/usr/bin/env python3
"""Verify core SD font glyph coverage and .cpfont size limits after build."""

from __future__ import annotations

import argparse
import json
import sys
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCRIPTS = ROOT / "scripts"
OUTPUT = ROOT / "lib/EpdFont/scripts/output"
MAX_FONT_BYTES = 25 * 1024 * 1024
EXPECTED_LATIN_GLYPHS = 1850
LEGACY_NOTO_URL = (
    "https://assets.crossmux.cn/fonts/m1-b4/releases/"
    "71141b4aeb4a3832824730fef73045f0d1c7f19c/NotoSansSC_12.cpfont"
)

sys.path.insert(0, str(SCRIPTS))
from cpfont_codepoints import cpfont_codepoints, cpfont_glyph_stats  # noqa: E402


def download_legacy(dest: Path) -> None:
    if dest.is_file() and dest.stat().st_size > 1_000_000:
        return
    dest.parent.mkdir(parents=True, exist_ok=True)
    urllib.request.urlretrieve(LEGACY_NOTO_URL, dest)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--legacy-noto", type=Path, help="Cached legacy NotoSansSC_12.cpfont")
    args = parser.parse_args()

    errors: list[str] = []

    for family in ("Literata", "Inter"):
        path = OUTPUT / family / f"{family}_12.cpfont"
        if not path.is_file():
            errors.append(f"missing {path}")
            continue
        _styles, glyphs, size = cpfont_glyph_stats(path)
        if glyphs != EXPECTED_LATIN_GLYPHS:
            errors.append(f"{family}_12: expected {EXPECTED_LATIN_GLYPHS} glyphs/style, got {glyphs}")

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

    legacy_path = args.legacy_noto or (ROOT / "test/fixtures/legacy/NotoSansSC_12.cpfont")
    try:
        download_legacy(legacy_path)
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
