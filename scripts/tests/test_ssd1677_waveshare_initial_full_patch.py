"""SSD1677 Waveshare cold-boot patch apply + verification."""

import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SDK = ROOT / "freeink-sdk"
sys.path.insert(0, str(ROOT / "scripts"))
from ssd1677_waveshare_initial_full_patch import apply_patch, verify_patch_applied

DRIVER_CPP = SDK / "libs/display/FreeInkDisplay/src/driver/CrossMuxSsd1677Driver.cpp"


def _reset_sdk_tree() -> None:
    subprocess.run(["git", "checkout", "--", "."], cwd=SDK, check=True, capture_output=True)
    for rej in SDK.rglob("*.rej"):
        rej.unlink()


class Ssd1677WavesharePatchTest(unittest.TestCase):
    def setUp(self) -> None:
        _reset_sdk_tree()

    def tearDown(self) -> None:
        _reset_sdk_tree()

    def test_fresh_apply_and_idempotent_reapply(self) -> None:
        apply_patch(ROOT)
        verify_patch_applied(SDK)
        apply_patch(ROOT)
        verify_patch_applied(SDK)
        self.assertFalse(list(SDK.rglob("*.rej")))

    def test_drifted_sdk_raises_without_promote_flag(self) -> None:
        text = DRIVER_CPP.read_text(encoding="utf-8")
        DRIVER_CPP.write_text(text.replace("value.halfRefreshTemp = 0x6A;", "value.halfRefreshTemp = 0x6B;"))
        with self.assertRaises(RuntimeError) as ctx:
            apply_patch(ROOT)
        self.assertIn("verification failed", str(ctx.exception).lower())
        self.assertNotIn("initialFastPromoteFull = true", DRIVER_CPP.read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
