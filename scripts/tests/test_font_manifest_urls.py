import re
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "lib/EpdFont/scripts"))

import cpfont_version  # noqa: E402


class FontManifestUrlTest(unittest.TestCase):
    def test_crossmux_endpoints_header_uses_dezzw_fonts_tag(self):
        header = (ROOT / "src/network/CrossMuxEndpoints.h").read_text()
        self.assertIn('FONT_GITHUB_REPO[] = "dezzw/crossmux"', header)
        self.assertIn("fonts-m%d-b%d", header)
        self.assertNotIn("crosspoint-fonts", header)
        self.assertNotIn("crossmux.com/api/assets/fonts", header)
        self.assertNotIn("assets.crossmux.cn", header)

    def test_stable_ota_manifest_uses_stable_release_tag(self):
        header = (ROOT / "src/network/CrossMuxEndpoints.h").read_text()
        self.assertIn("releases/download/stable/", header)
        self.assertNotIn("releases/latest", header)


if __name__ == "__main__":
    unittest.main()
