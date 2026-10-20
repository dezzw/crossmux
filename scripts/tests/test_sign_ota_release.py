#!/usr/bin/env python3
"""Tests for stable OTA manifest signing and release_notes embedding."""

import base64
import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

SCRIPTS = Path(__file__).resolve().parents[1]
ROOT = SCRIPTS.parent
sys.path.insert(0, str(SCRIPTS))

import generate_ota_notes
import sign_ota_release
import verify_nightly_release


VALID_NOTES = {
    'en': ['Stable note one', 'Stable note two'],
    'zh': ['稳定说明一', '稳定说明二'],
}


def notes_marker(notes=VALID_NOTES) -> str:
    return generate_ota_notes.marker(notes)


class SignOtaReleaseNotesTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.notes_path = self.root / 'ota-notes.md'
        self.notes_path.write_text(notes_marker(), encoding='utf-8')
        self.private_pem = (ROOT / 'test/fixtures/ota_test_private.pem').read_text()

    def write_manifest(self, flavor: str) -> Path:
        path = self.root / f'{flavor}-manifest.json'
        path.write_text(
            json.dumps(
                {
                    'version': '9.9.9-test',
                    'boardTag': 'waveshare_epaper_397',
                    'channel': 'stable',
                    'flavor': flavor,
                }
            )
            + '\n',
            encoding='utf-8',
        )
        return path

    def run_embed(self, manifest: Path) -> dict:
        with mock.patch.object(
            sys,
            'argv',
            [
                'sign_ota_release.py',
                '--manifest',
                str(manifest),
                '--release-notes',
                str(self.notes_path),
                '--embed-notes-only',
            ],
        ):
            sign_ota_release.main()
        return json.loads(manifest.read_text(encoding='utf-8'))

    def run_sign(self, manifest: Path, firmware: Path, output: Path, download_url: str) -> dict:
        with mock.patch.dict('os.environ', {'OTA_SIGNING_KEY': self.private_pem}):
            with mock.patch.object(
                sys,
                'argv',
                [
                    'sign_ota_release.py',
                    '--firmware',
                    str(firmware),
                    '--manifest',
                    str(manifest),
                    '--release-notes',
                    str(self.notes_path),
                    '--download-url',
                    download_url,
                    '--output',
                    str(output),
                ],
            ):
                sign_ota_release.main()
        return json.loads(output.read_text(encoding='utf-8'))

    def test_global_manifest_gets_english_release_notes(self):
        manifest = self.run_embed(self.write_manifest('global'))
        self.assertEqual(manifest['release_notes'], VALID_NOTES['en'])

    def test_cn_manifest_gets_chinese_release_notes(self):
        manifest = self.run_embed(self.write_manifest('zh-CN'))
        self.assertEqual(manifest['release_notes'], VALID_NOTES['zh'])

    def test_invalid_notes_file_fails(self):
        bad_notes = self.root / 'bad-notes.md'
        bad_notes.write_text('not a marker', encoding='utf-8')
        manifest = self.write_manifest('global')
        with mock.patch.object(
            sys,
            'argv',
            [
                'sign_ota_release.py',
                '--manifest',
                str(manifest),
                '--release-notes',
                str(bad_notes),
                '--embed-notes-only',
            ],
        ):
            with self.assertRaises(SystemExit):
                sign_ota_release.main()

    def test_signature_payload_unchanged_when_release_notes_added(self):
        firmware = self.root / 'firmware.bin'
        firmware.write_bytes(b'\x7fELF-test-firmware')
        manifest_in = self.write_manifest('global')
        signed_without = self.root / 'signed-without.json'
        signed_with = self.root / 'signed-with.json'
        download_url = 'https://github.com/example/releases/download/v9.9.9/firmware.bin'
        argv_base = [
            'sign_ota_release.py',
            '--firmware',
            str(firmware),
            '--manifest',
            str(manifest_in),
            '--download-url',
            download_url,
            '--output',
        ]
        with mock.patch.dict('os.environ', {'OTA_SIGNING_KEY': self.private_pem}):
            with mock.patch.object(sys, 'argv', [*argv_base, str(signed_without)]):
                sign_ota_release.main()
            with mock.patch.object(
                sys,
                'argv',
                [
                    *argv_base,
                    str(signed_with),
                    '--release-notes',
                    str(self.notes_path),
                ],
            ):
                sign_ota_release.main()
        without = json.loads(signed_without.read_text(encoding='utf-8'))
        with_notes = json.loads(signed_with.read_text(encoding='utf-8'))
        self.assertIn('release_notes', with_notes)

        def signing_digest(signed_manifest: dict) -> str:
            firmware_meta = signed_manifest['firmware']
            payload = sign_ota_release.canonical_payload(
                signed_manifest['version'],
                signed_manifest['boardTag'],
                firmware_meta['size'],
                firmware_meta['sha256'],
            )
            return hashlib.sha256(payload).hexdigest()

        self.assertEqual(signing_digest(without), signing_digest(with_notes))


class VerifyManifestReleaseNotesTest(unittest.TestCase):
    INDEX_NOTES = {
        'global': VALID_NOTES['en'],
        'zh-CN': VALID_NOTES['zh'],
    }

    def test_rejects_manifest_missing_release_notes(self):
        manifest = {'release_notes': []}
        with self.assertRaisesRegex(ValueError, 'missing release_notes'):
            verify_nightly_release.validate_manifest_release_notes(
                manifest, 'global', self.INDEX_NOTES
            )

    def test_accepts_matching_release_notes(self):
        verify_nightly_release.validate_manifest_release_notes(
            {'release_notes': list(VALID_NOTES['en'])},
            'global',
            self.INDEX_NOTES,
        )


if __name__ == '__main__':
    unittest.main()
