#!/usr/bin/env python3
"""Apply the Waveshare SSD1677 overlay, then run the FreeInk SDK host driver test."""

import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from ssd1677_waveshare_initial_full_patch import apply_patch

apply_patch(ROOT)
env = os.environ.copy()
env["CXX"] = env.get("CXX") or shutil.which("g++") or "g++"
subprocess.run(
    [sys.executable, str(ROOT / "freeink-sdk/libs/display/FreeInkDisplay/test/host/test_ssd1677.py")],
    check=True,
    env=env,
)
