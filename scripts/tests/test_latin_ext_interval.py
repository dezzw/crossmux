import sys
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[2] / "lib/EpdFont/scripts"
sys.path.insert(0, str(SCRIPTS))

import fontconvert_sdcard as fc  # noqa: E402


class LatinExtIntervalTest(unittest.TestCase):
    def test_latin_ext_includes_upstream_extras(self):
        merged = fc.resolve_intervals("latin-ext,greek,cyrillic,ipa-chars")
        flat = {cp for start, end in merged for cp in range(start, end + 1)}
        for cp in (0x20AC, 0x2103, 0x2116, 0x2212, 0x2014, 0x2026):
            self.assertIn(cp, flat, f"missing U+{cp:04X} in latin-ext merge")

    def test_literata_interval_preset_merges(self):
        merged = fc.resolve_intervals("latin-ext,greek,cyrillic,ipa-chars")
        self.assertGreaterEqual(len(merged), 8)


if __name__ == "__main__":
    unittest.main()
