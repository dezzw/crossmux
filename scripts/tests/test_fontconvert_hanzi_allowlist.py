import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[2] / "lib/EpdFont/scripts"
sys.path.insert(0, str(SCRIPTS))

import fontconvert_sdcard as fc  # noqa: E402


def _load_build_sd_fonts():
    spec = importlib.util.spec_from_file_location("build_sd_fonts", SCRIPTS / "build-sd-fonts.py")
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


class HanziAllowlistTest(unittest.TestCase):
    def test_committed_noto_allowlist(self):
        path = SCRIPTS / "noto_m1_b4_hanzi_coverage.txt"
        self.assertTrue(path.is_file(), "committed hanzi allowlist missing")
        allow = fc.load_hanzi_allowlist([path])
        self.assertGreater(len(allow), 8000)

    def test_ensure_noto_allowlist_noop_when_present(self):
        build = _load_build_sd_fonts()
        build.ensure_noto_hanzi_allowlist()
        self.assertTrue(build.NOTO_HANZI_ALLOWLIST.is_file())

    def test_hex_and_utf8_lines(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", delete=False) as handle:
            handle.write("4E00\n\u4e01\n")
            path = handle.name
        allow = fc.load_hanzi_allowlist([path])
        self.assertEqual(allow, {0x4E00, 0x4E01})
        Path(path).unlink(missing_ok=True)


if __name__ == "__main__":
    unittest.main()
