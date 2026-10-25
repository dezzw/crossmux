#!/usr/bin/env python3
"""Host-compile production CrossPointSettings.cpp and verify language JSON migration."""

import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BUILD_DIR = ROOT / "build" / "test"
TARGET = "CrossPointSettingsLanguageJsonTest"


class CrossPointSettingsLanguageResaveTest(unittest.TestCase):
    def test_from_json_language_codes(self):
        if not (BUILD_DIR / TARGET).exists():
            subprocess.run(
                ["cmake", "-S", str(ROOT / "test"), "-B", str(BUILD_DIR), "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release"],
                check=True,
            )
            subprocess.run(["cmake", "--build", str(BUILD_DIR), "--target", TARGET], check=True)
        exe = BUILD_DIR / "crosspoint_settings_language_json" / TARGET
        self.assertTrue(exe.exists(), msg=f"missing {exe}")
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    unittest.main()
