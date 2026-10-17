#!/usr/bin/env python3
"""Verify core SD font glyph coverage and .cpfont size limits after build."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
SCRIPTS = ROOT / "scripts"
OUTPUT = ROOT / "lib/EpdFont/scripts/output"
SCRIPT_DIR = ROOT / "lib/EpdFont/scripts"
MAX_FONT_BYTES = 25 * 1024 * 1024
EXPECTED_LATIN_GLYPHS = 1850
SD_FONTS = SCRIPT_DIR / "sd-fonts.yaml"

sys.path.insert(0, str(SCRIPTS))
from core_font_coverage import (  # noqa: E402
    codepoint_set_hash,
    legacy_bitmap_codepoints,
    load_pins,
    verify_legacy_bitmap_file,
)
from cpfont_codepoints import cpfont_codepoints, cpfont_glyph_stats  # noqa: E402


def _family_sizes(name: str) -> list[int]:
    config = yaml.safe_load(SD_FONTS.read_text(encoding="utf-8"))
    for family in config.get("families", []):
        if family["name"] == name:
            return [int(s) for s in family["sizes"]]
    raise KeyError(name)


def _style_ids(path: Path) -> list[int]:
    data = path.read_bytes()
    return [data[32 + i * 32] for i in range(data[12])]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--legacy-list", type=Path, help="Override legacy bitmap codepoint list")
    args = parser.parse_args()

    errors: list[str] = []
    pins = load_pins()

    sys.path.insert(0, str(SCRIPTS))
    from cpfont_reader import load_cpfont_style  # noqa: E402

    legacy_meta = pins["legacy_noto_m1_b4"]
    legacy_path = args.legacy_list or (SCRIPT_DIR / legacy_meta["file"])
    try:
        verify_legacy_bitmap_file(legacy_path, legacy_meta["sha256"])
        legacy_bitmap = legacy_bitmap_codepoints(legacy_path)
    except (OSError, ValueError) as exc:
        errors.append(f"legacy bitmap list: {exc}")
        legacy_bitmap = set()

    for family in ("Literata", "Inter"):
        expected_styles = pins[family]["styles"]
        for pt in _family_sizes(family):
            path = OUTPUT / family / f"{family}_{pt}.cpfont"
            if not path.is_file():
                errors.append(f"missing {path}")
                continue
            for sid in _style_ids(path):
                cps = cpfont_codepoints(path, style_id=sid)
                if len(cps) != EXPECTED_LATIN_GLYPHS:
                    errors.append(f"{path.name} style {sid}: expected {EXPECTED_LATIN_GLYPHS} glyphs, got {len(cps)}")
                digest = codepoint_set_hash(cps)
                if str(sid) not in expected_styles:
                    errors.append(f"{path.name} style {sid}: no pinned hash")
                elif digest != expected_styles[str(sid)]:
                    errors.append(f"{path.name} style {sid}: codepoint set hash mismatch")

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

    noto_pins = pins["NotoSansSC"]["styles"]
    noto_dir = OUTPUT / "NotoSansSC"
    if noto_dir.is_dir():
        for cpfont in sorted(noto_dir.glob("NotoSansSC_*.cpfont")):
            styles, _glyphs, size = cpfont_glyph_stats(cpfont)
            if size > MAX_FONT_BYTES:
                errors.append(f"{cpfont.name}: {size} bytes exceeds {MAX_FONT_BYTES} limit")
            if styles != 2:
                errors.append(f"{cpfont.name}: expected 2 styles (regular+bold), got {styles}")
            style_hashes: dict[int, str] = {}
            for sid in _style_ids(cpfont):
                cps = cpfont_codepoints(cpfont, style_id=sid)
                digest = codepoint_set_hash(cps)
                style_hashes[sid] = digest
                if str(sid) not in noto_pins:
                    errors.append(f"{cpfont.name} style {sid}: no pinned hash")
                elif digest != noto_pins[str(sid)]:
                    errors.append(f"{cpfont.name} style {sid}: codepoint set hash mismatch")
                if sid == 0 and legacy_bitmap:
                    with_bitmap = set(cpfont_codepoints(cpfont, style_id=0, require_bitmap=True))
                    missing = sorted(legacy_bitmap - with_bitmap)
                    if missing:
                        errors.append(
                            f"{cpfont.name} style 0 missing {len(missing)} legacy bitmap codepoints "
                            f"(first: {', '.join(hex(c) for c in missing[:8])})"
                        )
            if cpfont.name == "NotoSansSC_12.cpfont":
                reg = load_cpfont_style(cpfont, 0)
                bold = load_cpfont_style(cpfont, 1)
                gA_reg = reg.get_glyph(ord("A"))
                gA_bold = bold.get_glyph(ord("A"))
                if gA_reg and gA_bold and gA_reg.bitmap == gA_bold.bitmap:
                    errors.append("NotoSansSC_12 bold: Latin 'A' must differ from regular")
    else:
        errors.append(f"missing {noto_dir}")

    if errors:
        for err in errors:
            print(f"ERROR: {err}", file=sys.stderr)
        return 1

    print(json.dumps({"status": "ok", "latinGlyphsPerStyle": EXPECTED_LATIN_GLYPHS}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
