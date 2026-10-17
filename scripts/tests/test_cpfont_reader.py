#!/usr/bin/env python3
"""Regression tests for cpfont_reader (bitmap decode + layout)."""

from __future__ import annotations

import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "lib/EpdFont/scripts/output"
SCRIPTS = ROOT / "scripts"

import sys

sys.path.insert(0, str(SCRIPTS))
from cpfont_reader import (  # noqa: E402
    CpFontStyle,
    _coverage_2bit,
    coverage_draws_bw,
    draw_text_line,
    load_cpfont_style,
    pack_pil_mode1,
)


class CpfontReaderTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.literata = OUTPUT / "Literata/Literata_12.cpfont"
        if not cls.literata.is_file():
            raise unittest.SkipTest("built Literata_12.cpfont not present")

    def test_space_has_advance_without_bitmap(self) -> None:
        style = load_cpfont_style(self.literata, 0)
        space = style.get_glyph(0x20)
        self.assertIsNotNone(space)
        assert space is not None
        self.assertFalse(CpFontStyle.glyph_has_bitmap(space))
        self.assertGreater(space.advance_x, 0)

    def test_literata_bold_a_differs_from_A(self) -> None:
        bold = load_cpfont_style(self.literata, 1)
        ga = bold.get_glyph(ord("a"))
        gA = bold.get_glyph(ord("A"))
        self.assertIsNotNone(ga)
        self.assertIsNotNone(gA)
        assert ga is not None and gA is not None
        self.assertNotEqual(ga.bitmap, gA.bitmap)

    def test_draw_line_has_word_spacing(self) -> None:
        style = load_cpfont_style(self.literata, 0)
        w, h = 200, 40
        pixels = bytearray([1] * (w * h))
        draw_text_line(pixels, w, h, style, "a b", 0, style.ascender)
        packed = pack_pil_mode1(pixels, w, h)
        self.assertEqual(len(packed), (w * h + 7) // 8)
        # 'a' and 'b' should not share the same x column cluster
        cols_a = {x for y in range(h) for x in range(w) if pixels[y * w + x] == 0 and x < 20}
        self.assertTrue(cols_a)
        self.assertGreater(max(cols_a) - min(cols_a), 8)

    def test_coverage_matches_gfx_bw_threshold(self) -> None:
        self.assertFalse(coverage_draws_bw(0))
        self.assertTrue(coverage_draws_bw(1))
        self.assertTrue(coverage_draws_bw(3))
        # MSB-first 2-bit: byte 0xE4 -> pixels 3,1,0,...
        bmp = bytes([0xE4])
        self.assertEqual(_coverage_2bit(bmp, 0), 3)
        self.assertEqual(_coverage_2bit(bmp, 1), 2)


if __name__ == "__main__":
    unittest.main()
