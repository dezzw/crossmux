import configparser
import unittest
from pathlib import Path


class WaveshareSimulatorKeymapTest(unittest.TestCase):
    def test_simulator_env_enables_waveshare_device_and_gesture_include(self):
        config = configparser.ConfigParser(interpolation=None)
        config.read(Path(__file__).resolve().parents[2] / "platformio.ini")
        flags = config["env:simulator"]["build_flags"]
        self.assertIn("-DSIMULATOR_DEVICE_WAVESHARE_EPAPER_397=1", flags)
        self.assertNotIn("-DFREEINK_DEVICE_WAVESHARE_EPAPER_397=1", flags)
        self.assertIn("-Ilib/waveshare397_input/include", flags)
        self.assertIn("pre:scripts/patch_simulator_waveshare397.py", config["env:simulator"]["extra_scripts"])

    def test_overlay_and_bridge_sources_exist(self):
        root = Path(__file__).resolve().parents[2]
        self.assertTrue((root / "lib/simulator_waveshare397_overlay/HalGPIO.cpp").is_file())
        self.assertTrue((root / "lib/waveshare397_simulator_input/src/Waveshare397SimulatorInput.cpp").is_file())
        vendor = root / "lib/waveshare397_input/include/FunctionButtonGesture.h"
        self.assertTrue(vendor.is_file())
        self.assertIn(b"bootDialUsed_", vendor.read_bytes())


if __name__ == "__main__":
    unittest.main()
