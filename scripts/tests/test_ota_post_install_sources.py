"""Source guards for OTA post-install confirm, font unload, and glyph metadata validation."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class OtaPostInstallSourceGuards(unittest.TestCase):
    def test_go_home_confirms_running_image(self):
        source = (ROOT / "src/activities/ActivityManager.cpp").read_text()
        start = source.index("void ActivityManager::goHome(")
        end = source.index("\n}\n", start)
        body = source[start:end]
        self.assertIn("otaConfirmPendingRunningImageAtHome(renderer);", body)

    def test_main_confirms_when_not_in_recovery(self):
        source = (ROOT / "src/main.cpp").read_text()
        self.assertIn("if (fontsReady && !recoveryFirmwareMode)", source)
        self.assertIn("otaConfirmPendingRunningImageAtHome(renderer);", source)

    def test_ota_update_unloads_fonts_before_inactive_slot_write(self):
        source = (ROOT / "src/activities/settings/OtaUpdateActivity.cpp").read_text()
        install = source.index("updater.installUpdate(")
        prefix = source[:install]
        self.assertIn("sdFontSystem.releaseLoadedFont(renderer);", prefix)

    def test_read_glyph_metadata_validates_payload_size(self):
        source = (ROOT / "lib/EpdFont/SdCardFont.cpp").read_text()
        start = source.index("SdCardFont::readGlyphMetadata(")
        end = source.index("\n}\n", start)
        body = source[start:end]
        self.assertIn("glyphBitmapPayloadValid(glyph", body)


if __name__ == "__main__":
    unittest.main()
