#!/usr/bin/env python3
"""Regression wrapper for scripts/check_no_tracked_private_keys.py."""

import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class NoTrackedPrivateKeysTest(unittest.TestCase):
    def test_repo_has_no_tracked_private_pem(self):
        proc = subprocess.run(
            [sys.executable, str(ROOT / 'scripts' / 'check_no_tracked_private_keys.py')],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        self.assertEqual(proc.returncode, 0, proc.stderr or proc.stdout)


if __name__ == '__main__':
    unittest.main()
