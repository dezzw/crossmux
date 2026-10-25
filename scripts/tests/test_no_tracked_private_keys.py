#!/usr/bin/env python3
"""Tests for scripts/check_no_tracked_private_keys.py."""

from __future__ import annotations

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))

import check_no_tracked_private_keys as scanner


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
            repo = Path(tmp)
            subprocess.run(['git', 'init', '-q'], cwd=repo, check=True)
            subprocess.run(['git', 'config', 'user.email', 'test@example.com'], cwd=repo, check=True)
            subprocess.run(['git', 'config', 'user.name', 'test'], cwd=repo, check=True)
            clean = repo / 'readme.md'
            clean.write_text('no secrets\n')
            bad = repo / 'leak.key'
            bad.write_text(_minimal_ec_private_pem())
            subprocess.run(['git', 'add', '.'], cwd=repo, check=True)
            offenders = scanner.scan_tracked_files(repo)
            self.assertEqual(offenders, ['leak.key'])

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
