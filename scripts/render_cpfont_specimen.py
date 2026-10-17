#!/usr/bin/env python3
"""Render PR #23 font specimens from built .cpfont files (800×480, 1-bit)."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "docs/screenshots/pr23"
OUTPUT_DIR = ROOT / "lib/EpdFont/scripts/output"

sys.path.insert(0, str(ROOT / "scripts"))
from cpfont_reader import draw_text_line, load_cpfont_style, pack_pil_mode1, wrap_text  # noqa: E402

SCREEN_W, SCREEN_H = 800, 480
MARGIN = 16
CONTENT_W = SCREEN_W - 2 * MARGIN

BODY_EN = (
    "Reading on e-ink rewards calm pacing and clear typography. "
    "The quick brown fox jumps over the lazy dog while the reader "
    "tracks every word without distraction."
)
MIXED_LINE = "“”—…€℃№①② and mixed 中文 Bold 加粗 with Latin."
BODY_CN = "中文正文示例：阅读器使用真实 Bold 字重显示加粗，Italic 仍映射 Regular 字模。"


def label_font() -> ImageFont.ImageFont:
    try:
        return ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 13)
    except OSError:
        return ImageFont.load_default()


def render_page(sections: list[tuple[str, object, str]], title: str, pt: int, out_path: Path) -> None:
    pixels = bytearray([1] * (SCREEN_W * SCREEN_H))
    label = label_font()
    label_lines: list[tuple[int, str]] = []
    y = MARGIN + 18

    for caption, style, text in sections:
        if y > SCREEN_H - 48:
            break
        label_lines.append((y, caption))
        y += 14
        baseline = y + style.ascender
        for line in wrap_text(style, text, CONTENT_W):
            if baseline + max(style.descender, 0) > SCREEN_H - MARGIN:
                break
            draw_text_line(pixels, SCREEN_W, SCREEN_H, style, line, MARGIN, baseline)
            baseline += style.advance_y
        y = baseline + 10

    img = Image.frombytes("1", (SCREEN_W, SCREEN_H), pack_pil_mode1(pixels, SCREEN_W, SCREEN_H))
    draw = ImageDraw.Draw(img)
    draw.text((MARGIN, MARGIN), f"{title} @ {pt}pt — .cpfont specimen", fill=0, font=label)
    for ly, caption in label_lines:
        draw.text((MARGIN, ly), caption, fill=0, font=label)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    img.save(out_path)
    print(f"Wrote {out_path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=OUT)
    parser.add_argument("--input", type=Path, default=OUTPUT_DIR)
    args = parser.parse_args()

    for pt in (12, 14, 16, 18):
        path = args.input / "NotoSansSC" / f"NotoSansSC_{pt}.cpfont"
        reg = load_cpfont_style(path, 0)
        bold = load_cpfont_style(path, 1)
        render_page(
            [
                ("Regular — body", reg, f"{BODY_CN} {MIXED_LINE}"),
                ("Bold — body", bold, f"{BODY_CN} {MIXED_LINE}"),
            ],
            "NotoSansSC",
            pt,
            args.output / f"notosans_sc_{pt}.png",
        )

    for pt in (12, 14, 16):
        lit_path = args.input / "Literata" / f"Literata_{pt}.cpfont"
        int_path = args.input / "Inter" / f"Inter_{pt}.cpfont"
        lit_r, lit_b, lit_i = load_cpfont_style(lit_path, 0), load_cpfont_style(lit_path, 1), load_cpfont_style(lit_path, 2)
        int_r, int_b, int_i = load_cpfont_style(int_path, 0), load_cpfont_style(int_path, 1), load_cpfont_style(int_path, 2)
        sym = MIXED_LINE
        render_page(
            [
                ("Literata regular", lit_r, f"{BODY_EN} {sym}"),
                ("Literata bold", lit_b, f"{BODY_EN} {sym}"),
                ("Literata italic", lit_i, f"{BODY_EN} {sym}"),
            ],
            "Literata",
            pt,
            args.output / f"literata_{pt}.png",
        )
        render_page(
            [
                ("Inter regular", int_r, f"{BODY_EN} {sym}"),
                ("Inter bold", int_b, f"{BODY_EN} {sym}"),
                ("Inter italic", int_i, f"{BODY_EN} {sym}"),
            ],
            "Inter",
            pt,
            args.output / f"inter_{pt}.png",
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
