#!/usr/bin/env python3
"""Print NotoSansSC release asset sizes: crossmux.com m1-b4 vs local build."""

from __future__ import annotations

import json
import struct
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "lib/EpdFont/scripts/output"

# crossmux.com m1-b4 NotoSansSC (single regular style), from fonts.json 2026-10-17.
LEGACY_NOTO_SIZES: dict[str, int] = {
    "NotoSansSC_8.cpfont": 867_664,
    "NotoSansSC_10.cpfont": 1_161_151,
    "NotoSansSC_12.cpfont": 1_533_978,
    "NotoSansSC_14.cpfont": 1_984_438,
    "NotoSansSC_16.cpfont": 2_479_612,
    "NotoSansSC_18.cpfont": 3_037_041,
}
LEGACY_NOTO_12_GLYPHS = 9680
LEGACY_NOTO_12_STYLES = 1


def glyph_stats(path: Path) -> tuple[int, int]:
    data = path.read_bytes()
    styles = data[12]
    glyphs = []
    for i in range(styles):
        _, _, gc = struct.unpack_from("<B3xII", data, 32 + i * 32)
        glyphs.append(gc)
    return styles, max(glyphs) if glyphs else 0


def load_legacy_noto_sizes() -> dict[str, int]:
    try:
        manifest = json.load(
            urllib.request.urlopen("https://crossmux.com/api/assets/fonts/m1-b4/fonts.json", timeout=60)
        )
        return {
            f["name"]: f["size"]
            for fam in manifest["families"]
            if fam["name"] == "NotoSansSC"
            for f in fam["files"]
        }
    except OSError:
        return dict(LEGACY_NOTO_SIZES)


def print_family_table(family: str, legacy_sizes: dict[str, int] | None = None) -> None:
    build_dir = OUTPUT / family
    if legacy_sizes:
        print(f"\n{family} .cpfont bytes (legacy vs PR23 build):")
        print(f"{'size':>6}  {'legacy':>12}  {'PR23':>12}  {'delta':>10}  glyphs(PR23)")
    else:
        print(f"\n{family} .cpfont bytes (PR23 build):")
        print(f"{'size':>6}  {'PR23':>12}  glyphs(PR23)")
    for pt in (8, 10, 12, 14, 16, 18):
        name = f"{family}_{pt}.cpfont"
        new_path = build_dir / name
        if not new_path.is_file():
            if pt in (8, 10) and family != "NotoSansSC":
                continue
            if family == "NotoSansSC" or pt in (12, 14, 16, 18):
                print(f"{pt:>6}pt  (missing {name})")
            continue
        new_size = new_path.stat().st_size
        styles, glyphs = glyph_stats(new_path)
        if legacy_sizes:
            legacy = legacy_sizes.get(name, 0)
            delta = new_size - legacy if legacy else 0
            print(f"{pt:>6}pt  {legacy:>12,}  {new_size:>12,}  {delta:>+10,}  {glyphs} x {styles} styles")
        else:
            print(f"{pt:>6}pt  {new_size:>12,}  {glyphs} x {styles} styles")


def main() -> None:
    old = load_legacy_noto_sizes()
    print("NotoSansSC glyph coverage @ 12pt:")
    build_12 = OUTPUT / "NotoSansSC/NotoSansSC_12.cpfont"
    if build_12.is_file():
        styles, glyphs = glyph_stats(build_12)
        print(f"  legacy: {LEGACY_NOTO_12_GLYPHS} glyphs x {LEGACY_NOTO_12_STYLES} style (manifest)")
        print(f"  PR23:   {glyphs} glyphs x {styles} styles")
    print_family_table("NotoSansSC", old)
    print_family_table("Literata")
    print_family_table("Inter")


if __name__ == "__main__":
    main()
