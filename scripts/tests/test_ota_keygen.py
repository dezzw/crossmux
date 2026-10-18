#!/usr/bin/env python3
"""Tests for scripts/ota_keygen.py header formatting."""

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS))

import ota_keygen


class OtaKeygenHeaderTest(unittest.TestCase):
    def test_format_header_has_no_trailing_blank_line(self):
        with tempfile.TemporaryDirectory() as tmp:
            private_path = Path(tmp) / 'private.pem'
            subprocess.check_call(
                ['openssl', 'ecparam', '-name', 'prime256v1', '-genkey', '-noout', '-out', str(private_path)]
            )
            public_pem = subprocess.check_output(
                ['openssl', 'ec', '-in', str(private_path), '-pubout'],
                text=True,
            )

        header = ota_keygen.format_ota_signing_header(public_pem)
        self.assertTrue(header.endswith(';\n'), 'header must end with semicolon and single newline')
        self.assertFalse(header.endswith(';\n\n'), 'header must not have trailing blank line')
        self.assertNotEqual(header.splitlines()[-1], '')


if __name__ == '__main__':
    unittest.main()
