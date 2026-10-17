#!/usr/bin/env python3
"""Render reader-style font specimens for PR #23 (800×480, 1-bit style)."""

from __future__ import annotations

import argparse
from pathlib import Path

import freetype
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "docs/screenshots/pr23"
EPDFONT = ROOT / "lib/EpdFont"
SCRIPTS = EPDFONT / "scripts"


def load_face(path: Path, size: int) -> freetype.Face:
    face = freetype.Face(str(path))
    face.set_char_size(size * 64, size * 64, 150, 150)
    return face


def draw_text(draw: ImageDraw.ImageDraw, face: freetype.Face, x: int, y: int, text: str) -> int:
    cursor = x
    for ch in text:
        face.load_char(ord(ch), freetype.FT_LOAD_RENDER)
        bitmap = face.glyph.bitmap
        pitch = abs(bitmap.pitch)
        for row in range(bitmap.rows):
            for col in range(bitmap.width):
                if bitmap.buffer[row * pitch + col]:
                    draw.point((cursor + face.glyph.bitmap_left + col, y + face.glyph.bitmap_top + row), fill=0)
        cursor += face.glyph.advance.x >> 6
    return cursor


def save_specimen(path: Path, title: str, size: int, lines: list[tuple[str, freetype.Face]]):
    img = Image.new("1", (800, 480), 1)
    draw = ImageDraw.Draw(img)
    try:
        label = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14)
    except OSError:
        label = ImageFont.load_default()
    draw.text((16, 8), f"{title} @ {size}pt (simulator viewport)", fill=0, font=label)
    y = 40
    for caption, face in lines:
        draw.text((16, y), caption, fill=0, font=label)
        y += 18
        draw_text(draw, face, 24, y, lines_text(caption))
        y += size + 20
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path)
    print(f"Wrote {path}")


def lines_text(caption: str) -> str:
    if caption.startswith("CN"):
        return "中文阅读正文：真实 Bold 字重对比（Italic 仍用 Regular 字模）。"
    if "Literata" in caption:
        return "The quick brown fox jumps over the lazy dog."
    return "The quick brown fox jumps over the lazy dog."


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=OUT)
    args = parser.parse_args()

    noto_r = EPDFONT / "builtinFonts/source/NotoSansSC/NotoSansSC-Regular.otf"
    noto_b = EPDFONT / "builtinFonts/source/NotoSansSC/NotoSansSC-Bold.otf"
    lit_r = SCRIPTS / "downloaded_fonts/Literata/Literata-Regular.ttf"
    lit_b = SCRIPTS / "downloaded_fonts/Literata/Literata-Bold.ttf"
    lit_i = SCRIPTS / "downloaded_fonts/Literata/Literata-Italic.ttf"
    int_dir = SCRIPTS / "instanced_fonts/Inter"
    int_r = next(int_dir.glob("regular_*.ttf"))
    int_b = next(int_dir.glob("bold_*.ttf"))
    int_i = next(int_dir.glob("italic_*.ttf"))

    for pt in (12, 14, 16, 18):
        save_specimen(
            args.output / f"notosans_sc_{pt}.png",
            "NotoSansSC",
            pt,
            [
                ("CN regular", load_face(noto_r, pt)),
                ("CN bold", load_face(noto_b, pt)),
                ("CN italic (Regular master)", load_face(noto_r, pt)),
            ],
        )

    for pt in (12, 14, 16):
        save_specimen(
            args.output / f"literata_{pt}.png",
            "Literata",
            pt,
            [
                ("Literata regular", load_face(lit_r, pt)),
                ("Literata bold", load_face(lit_b, pt)),
                ("Literata italic", load_face(lit_i, pt)),
            ],
        )
        save_specimen(
            args.output / f"inter_{pt}.png",
            "Inter",
            pt,
            [
                ("Inter regular", load_face(int_r, pt)),
                ("Inter bold", load_face(int_b, pt)),
                ("Inter italic", load_face(int_i, pt)),
            ],
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
