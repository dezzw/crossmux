"""SecureNet overlay stamp and --check mode."""

import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class SecureNetOverlayCheckTest(unittest.TestCase):
    def test_check_passes_on_committed_overlay(self):
        subprocess.check_call(["python3", "scripts/seed_securenet_overlay.py", "--check"], cwd=ROOT)


if __name__ == "__main__":
    unittest.main()
