#!/usr/bin/env python3
"""Tests for scripts/check_no_tracked_private_keys.py."""

from __future__ import annotations

import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))

import check_no_tracked_private_keys as scanner

OPENSSL_REQUIRED_MSG = 'openssl is required for private-key scanner tests'


def _require_openssl() -> None:
    if shutil.which('openssl') is None:
        raise unittest.SkipTest(OPENSSL_REQUIRED_MSG)


def _pem_header(*words: str) -> str:
    """Assemble a PEM BEGIN line from fragments (avoids a contiguous marker in source)."""
    inner = ' '.join(words)
    return f'-----BEGIN {inner}-----'


def _pem_footer(*words: str) -> str:
    inner = ' '.join(words)
    return f'-----END {inner}-----'


def _minimal_ec_private_pem() -> str:
    return '\n'.join(
        [
            _pem_header('EC', 'PRIVATE', 'KEY'),
            'MOCKBASE64DATA',
            _pem_footer('EC', 'PRIVATE', 'KEY'),
            '',
        ]
    )


def _generate_ec_der_samples() -> tuple[bytes, bytes, bytes]:
    _require_openssl()
    with tempfile.TemporaryDirectory() as tmp:
        ec_pem = Path(tmp) / 'ec.pem'
        subprocess.check_call(
            ['openssl', 'ecparam', '-name', 'prime256v1', '-genkey', '-noout', '-out', str(ec_pem)]
        )
        sec1 = subprocess.check_output(['openssl', 'ec', '-in', str(ec_pem), '-outform', 'DER'])
        pkcs8 = subprocess.check_output(
            ['openssl', 'pkcs8', '-topk8', '-nocrypt', '-in', str(ec_pem), '-outform', 'DER']
        )
        spki = subprocess.check_output(['openssl', 'ec', '-in', str(ec_pem), '-pubout', '-outform', 'DER'])
    return sec1, pkcs8, spki


def _generate_rsa_pkcs1_der() -> bytes:
    _require_openssl()
    with tempfile.TemporaryDirectory() as tmp:
        rsa_pem = Path(tmp) / 'rsa.pem'
        subprocess.check_call(['openssl', 'genrsa', '-out', str(rsa_pem), '2048'])
        return subprocess.check_output(['openssl', 'rsa', '-in', str(rsa_pem), '-outform', 'DER'])


def _init_temp_repo(base: Path) -> Path:
    subprocess.run(['git', 'init', '-q'], cwd=base, check=True)
    subprocess.run(['git', 'config', 'user.email', 'test@example.com'], cwd=base, check=True)
    subprocess.run(['git', 'config', 'user.name', 'test'], cwd=base, check=True)
    return base


class PrivateKeyScannerTest(unittest.TestCase):
    def test_detects_ec_private_in_pem(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'secret.pem'
            path.write_text(_minimal_ec_private_pem())
            self.assertTrue(scanner.file_contains_private_key_block(path.read_text()))

    def test_detects_rsa_private_in_key_extension(self):
        body = '\n'.join([_pem_header('RSA', 'PRIVATE', 'KEY'), 'x', _pem_footer('RSA', 'PRIVATE', 'KEY')])
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'legacy.key'
            path.write_text(body + '\n')
            self.assertTrue(scanner.file_contains_private_key_block(path.read_text()))

    def test_detects_encrypted_private_in_txt(self):
        body = '\n'.join(
            [
                _pem_header('ENCRYPTED', 'PRIVATE', 'KEY'),
                'cipher',
                _pem_footer('ENCRYPTED', 'PRIVATE', 'KEY'),
            ]
        )
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'notes.txt'
            path.write_text(body + '\n')
            self.assertTrue(scanner.file_contains_private_key_block(path.read_text()))

    def test_detects_mixed_case_marker_in_uppercase_pem(self):
        begin = _pem_header('ec', 'PRIVATE', 'key')
        end = _pem_footer('ec', 'PRIVATE', 'key')
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'WEIRD.PEM'
            path.write_text(f'{begin}\ndata\n{end}\n')
            self.assertTrue(scanner.file_contains_private_key_block(path.read_text()))

    def test_detects_private_block_in_pem_bak(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'backup.pem.bak'
            path.write_text(_minimal_ec_private_pem())
            self.assertTrue(scanner.file_contains_private_key_block(path.read_text()))

    def test_detects_private_block_embedded_in_python_source(self):
        pem = _minimal_ec_private_pem()
        py_body = 'CONFIG = """\n' + pem + '"""\n'
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'embedded.py'
            path.write_text(py_body)
            self.assertTrue(scanner.file_contains_private_key_block(path.read_text()))

    def test_scan_tracked_files_in_temp_repo(self):
        with tempfile.TemporaryDirectory() as tmp:
            repo = _init_temp_repo(Path(tmp))
            clean = repo / 'readme.md'
            clean.write_text('no secrets\n')
            bad = repo / 'leak.key'
            bad.write_text(_minimal_ec_private_pem())
            subprocess.run(['git', 'add', '.'], cwd=repo, check=True)
            offenders = scanner.scan_tracked_files(repo)
            self.assertEqual(offenders, ['leak.key'])

    def _assert_blocked_extension(self, filename: str) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = _init_temp_repo(Path(tmp))
            (repo / filename).write_bytes(b'not-a-real-key')
            subprocess.run(['git', 'add', '.'], cwd=repo, check=True)
            offenders = scanner.scan_tracked_files(repo)
            self.assertEqual(offenders, [filename])

    def test_blocks_tracked_der_extension(self):
        self._assert_blocked_extension('blob.der')

    def test_blocks_tracked_p12_extension(self):
        self._assert_blocked_extension('store.P12')

    def test_blocks_tracked_pfx_extension(self):
        self._assert_blocked_extension('legacy.pfx')

    def test_blocks_tracked_pk8_extension(self):
        self._assert_blocked_extension('key.pk8')

    def test_blocks_tracked_ppk_extension(self):
        self._assert_blocked_extension('ssh.ppk')

    def test_blocks_tracked_key_extension(self):
        self._assert_blocked_extension('material.KEY')

    def test_detects_sec1_ec_der_magic(self):
        sec1, _, _ = _generate_ec_der_samples()
        self.assertTrue(scanner.looks_like_sec1_ec_private(sec1))

    def test_detects_pkcs8_ec_der_magic(self):
        _, pkcs8, _ = _generate_ec_der_samples()
        self.assertTrue(scanner.looks_like_pkcs8_ec_private(pkcs8))

    def test_detects_rsa_pkcs1_der_magic(self):
        rsa_der = _generate_rsa_pkcs1_der()
        self.assertTrue(scanner.looks_like_rsa_pkcs1_private(rsa_der))

    def test_public_spki_der_not_flagged(self):
        _, _, spki = _generate_ec_der_samples()
        self.assertFalse(scanner.file_has_der_private_magic(spki))

    def test_random_binary_not_flagged(self):
        self.assertFalse(scanner.file_has_der_private_magic(b'\x00\x01\x02hello world'))

    def test_scan_tracked_sec1_der_without_blocked_extension(self):
        sec1, _, _ = _generate_ec_der_samples()
        with tempfile.TemporaryDirectory() as tmp:
            repo = _init_temp_repo(Path(tmp))
            (repo / 'payload.bin').write_bytes(sec1)
            subprocess.run(['git', 'add', '.'], cwd=repo, check=True)
            offenders = scanner.scan_tracked_files(repo)
            self.assertEqual(offenders, ['payload.bin'])

    def test_repo_has_no_tracked_private_blocks(self):
        proc = subprocess.run(
            [sys.executable, str(ROOT / 'scripts' / 'check_no_tracked_private_keys.py')],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        self.assertEqual(proc.returncode, 0, proc.stderr or proc.stdout)


if __name__ == '__main__':
    unittest.main()
