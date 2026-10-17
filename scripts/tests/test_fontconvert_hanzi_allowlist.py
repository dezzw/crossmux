import sys
import tempfile
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[2] / "lib/EpdFont/scripts"
sys.path.insert(0, str(SCRIPTS))

import fontconvert_sdcard as fc  # noqa: E402


class HanziAllowlistTest(unittest.TestCase):
    def test_gb2312_file_has_expected_count(self):
        path = SCRIPTS / "chinese_gb2312_hanzi.txt"
        allow = fc.load_hanzi_allowlist([path])
        self.assertEqual(len(allow), 6763)

    def test_hex_and_utf8_lines(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", delete=False) as handle:
            handle.write("4E00\n\u4e01\n")
            path = handle.name
        allow = fc.load_hanzi_allowlist([path])
        self.assertEqual(allow, {0x4E00, 0x4E01})
        Path(path).unlink(missing_ok=True)


if __name__ == "__main__":
    unittest.main()
