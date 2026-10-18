"""Regression: HalDisplay must forward grayscale base refreshes to the SDK."""

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HAL_DISPLAY = ROOT / "lib" / "hal" / "HalDisplay.cpp"


class HalDisplayGrayscaleBaseTest(unittest.TestCase):
    def test_display_grayscale_base_forwards_to_eink(self):
        source = HAL_DISPLAY.read_text(encoding="utf-8")
        self.assertIn("einkDisplay.displayGrayscaleBase", source)
        self.assertNotRegex(
            source,
            r"void HalDisplay::displayGrayscaleBase\([^{]*\)\s*\{\s*\}",
            msg="displayGrayscaleBase must not be an empty stub",
        )


if __name__ == "__main__":
    unittest.main()
