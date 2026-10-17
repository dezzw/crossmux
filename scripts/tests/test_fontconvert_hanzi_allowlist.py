import sys
import tempfile
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[2] / "lib/EpdFont/scripts"
sys.path.insert(0, str(SCRIPTS))

import fontconvert_sdcard as fc  # noqa: E402


class HanziAllowlistTest(unittest.TestCase):
    def test_noto_legacy_hanzi_file_non_empty(self):
        path = SCRIPTS / "noto_m1_b4_hanzi_coverage.txt"
        allow = fc.load_hanzi_allowlist([path])
        self.assertGreater(len(allow), 8000)

    def test_hex_and_utf8_lines(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", delete=False) as handle:
            handle.write("4E00\n\u4e01\n")
            path = handle.name
        allow = fc.load_hanzi_allowlist([path])
        self.assertEqual(allow, {0x4E00, 0x4E01})
        Path(path).unlink(missing_ok=True)


if __name__ == "__main__":
    unittest.main()
