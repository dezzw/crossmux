#!/usr/bin/env python3
"""Regression tests for scripts/verify_ota_public_key.py C-string PEM decoding."""

import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

SCRIPTS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS))

import verify_ota_public_key as verify


def _generate_ec_keypair() -> tuple[str, str]:
    with tempfile.TemporaryDirectory() as tmp:
        private_path = Path(tmp) / 'private.pem'
        subprocess.check_call(
            ['openssl', 'ecparam', '-name', 'prime256v1', '-genkey', '-noout', '-out', str(private_path)]
        )
        private_pem = private_path.read_text()
        public_pem = subprocess.check_output(
            ['openssl', 'ec', '-in', str(private_path), '-pubout'],
            text=True,
        )
    return private_pem, public_pem


def _format_ota_signing_header(public_pem: str) -> str:
    lines = public_pem.strip().splitlines()
    body = '\n'.join(f'    "{line}\\n"' for line in lines)
    return (
        '#pragma once\n\n'
        'inline constexpr char OTA_SIGNING_PUBLIC_KEY_PEM[] =\n'
        f'{body};\n'
    )


class VerifyOtaPublicKeyTest(unittest.TestCase):
    def test_decode_c_string_escapes_newlines(self):
        self.assertEqual(
            verify.decode_c_string_escapes('-----BEGIN PUBLIC KEY-----\\nMFkw\\n-----END PUBLIC KEY-----\\n'),
            '-----BEGIN PUBLIC KEY-----\nMFkw\n-----END PUBLIC KEY-----\n',
        )

    def test_matching_key_passes(self):
        private_pem, public_pem = _generate_ec_keypair()
        header_text = _format_ota_signing_header(public_pem)
        with tempfile.NamedTemporaryFile('w', suffix='.h', delete=False) as header_file:
            header_file.write(header_text)
            header_path = Path(header_file.name)
        try:
            with mock.patch.object(verify, 'HEADER', header_path):
                with mock.patch.dict(os.environ, {'OTA_SIGNING_KEY': private_pem}, clear=False):
                    self.assertEqual(verify.main(), 0)
        finally:
            header_path.unlink(missing_ok=True)

    def test_mismatched_key_fails(self):
        private_pem, _ = _generate_ec_keypair()
        _, other_public = _generate_ec_keypair()
        header_text = _format_ota_signing_header(other_public)
        with tempfile.NamedTemporaryFile('w', suffix='.h', delete=False) as header_file:
            header_file.write(header_text)
            header_path = Path(header_file.name)
        try:
            with mock.patch.object(verify, 'HEADER', header_path):
                with mock.patch.dict(os.environ, {'OTA_SIGNING_KEY': private_pem}, clear=False):
                    self.assertEqual(verify.main(), 1)
        finally:
            header_path.unlink(missing_ok=True)

    def test_void_committed_test_key_rejected(self):
        # Former repo test key (public and void; PEM removed from git).
        private_pem = """-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIPvmyRwA0eTSPnCVwQdu5i+5EkxUqlfyvEgQK7oyAUx1oAoGCCqGSM49
AwEHoUQDQgAENweb3c1fWGTjswl48pR9Cft06rNAZwDkN8W4CaTwBlsxmqJbDBCV
lfYWni9HKeb6ypduBAMCCwpr+iv3E47ZzQ==
-----END EC PRIVATE KEY-----
"""
        public_pem = subprocess.check_output(['openssl', 'ec', '-pubout'], input=private_pem, text=True)
        header_text = _format_ota_signing_header(public_pem)
        with tempfile.NamedTemporaryFile('w', suffix='.h', delete=False) as header_file:
            header_file.write(header_text)
            header_path = Path(header_file.name)
        try:
            with mock.patch.object(verify, 'HEADER', header_path):
                with mock.patch.dict(os.environ, {'OTA_SIGNING_KEY': private_pem}, clear=False):
                    rc = verify.main()
                    self.assertEqual(rc, 1)
        finally:
            header_path.unlink(missing_ok=True)


if __name__ == '__main__':
    unittest.main()
