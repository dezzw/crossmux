#!/usr/bin/env python3
"""Offline checks for core font coverage pins (no font masters or network)."""

from __future__ import annotations

import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = ROOT / "scripts"
SCRIPT_DIR = ROOT / "lib/EpdFont/scripts"

import sys

sys.path.insert(0, str(SCRIPTS))
from core_font_coverage import legacy_bitmap_codepoints, load_pins, verify_legacy_bitmap_file  # noqa: E402


class CoreFontCoverageOfflineTest(unittest.TestCase):
    def test_legacy_bitmap_list_sha256(self) -> None:
        pins = load_pins()
        meta = pins["legacy_noto_m1_b4"]
        path = SCRIPT_DIR / meta["file"]
        verify_legacy_bitmap_file(path, meta["sha256"])
        cps = legacy_bitmap_codepoints(path)
        self.assertGreater(len(cps), 9000)

    def test_coverage_pins_have_all_families(self) -> None:
        pins = load_pins()
        for key in ("Literata", "Inter", "NotoSansSC"):
            self.assertIn("styles", pins[key])
            self.assertGreaterEqual(len(pins[key]["styles"]), 2)


if __name__ == "__main__":
    unittest.main()
