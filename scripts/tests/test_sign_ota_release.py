#!/usr/bin/env python3
"""Tests for stable OTA manifest signing and release_notes verification."""

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
import ota_test_fixtures
import sign_ota_release


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
        self.private_pem = ota_test_fixtures.read_test_private_pem()

    def write_manifest(self, flavor: str = 'global') -> Path:
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

    def run_sign(self, manifest: Path, firmware: Path, output: Path, download_url: str, notes=True) -> dict:
        argv = [
            'sign_ota_release.py',
            '--firmware',
            str(firmware),
            '--manifest',
            str(manifest),
            '--download-url',
            download_url,
            '--output',
            str(output),
        ]
        if notes:
            argv.extend(['--release-notes', str(self.notes_path)])
        with mock.patch.dict('os.environ', {'OTA_SIGNING_KEY': self.private_pem}):
            with mock.patch.object(sys, 'argv', argv):
                sign_ota_release.main()
        return json.loads(output.read_text(encoding='utf-8'))

    def test_global_manifest_gets_english_release_notes(self):
        firmware = self.root / 'firmware.bin'
        firmware.write_bytes(b'fw')
        output = self.root / 'signed.json'
        manifest = self.run_sign(
            self.write_manifest('global'),
            firmware,
            output,
            'https://github.com/example/releases/download/v9.9.9/firmware.bin',
        )
        self.assertEqual(manifest['release_notes'], VALID_NOTES['en'])

    def test_invalid_notes_file_fails_during_sign(self):
        bad_notes = self.root / 'bad-notes.md'
        bad_notes.write_text('not a marker', encoding='utf-8')
        firmware = self.root / 'firmware.bin'
        firmware.write_bytes(b'fw')
        with mock.patch.dict('os.environ', {'OTA_SIGNING_KEY': self.private_pem}):
            with mock.patch.object(
                sys,
                'argv',
                [
                    'sign_ota_release.py',
                    '--firmware',
                    str(firmware),
                    '--manifest',
                    str(self.write_manifest('global')),
                    '--release-notes',
                    str(bad_notes),
                    '--download-url',
                    'https://example.com/firmware.bin',
                    '--output',
                    str(self.root / 'out.json'),
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

    def test_signed_manifest_json_preserves_chinese_in_release_notes(self):
        firmware = self.root / 'firmware.bin'
        firmware.write_bytes(b'fw')
        output = self.root / 'signed.json'
        notes_path = self.root / 'zh-notes.md'
        notes_path.write_text(notes_marker(), encoding='utf-8')
        manifest_in = self.write_manifest('zh-CN')
        with mock.patch.dict('os.environ', {'OTA_SIGNING_KEY': self.private_pem}):
            with mock.patch.object(
                sys,
                'argv',
                [
                    'sign_ota_release.py',
                    '--firmware',
                    str(firmware),
                    '--manifest',
                    str(manifest_in),
                    '--release-notes',
                    str(notes_path),
                    '--download-url',
                    'https://example.com/firmware.bin',
                    '--output',
                    str(output),
                ],
            ):
                sign_ota_release.main()
        raw = output.read_text(encoding='utf-8')
        self.assertIn('稳定说明一', raw)
        self.assertNotIn('\\u7a33', raw)


class VerifyGlobalManifestReleaseNotesTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.notes_path = self.root / 'ota-notes.md'
        self.notes_path.write_text(notes_marker(), encoding='utf-8')

    def test_accepts_matching_release_notes(self):
        sign_ota_release.verify_global_manifest_release_notes(
            {'release_notes': list(VALID_NOTES['en'])},
            self.notes_path,
        )

    def test_rejects_mismatch(self):
        with self.assertRaisesRegex(ValueError, 'do not match'):
            sign_ota_release.verify_global_manifest_release_notes(
                {'release_notes': ['Wrong one', 'Wrong two']},
                self.notes_path,
            )

    def test_rejects_invalid_note(self):
        manifest = {'release_notes': list(VALID_NOTES['en'])}
        with mock.patch.object(
            generate_ota_notes,
            'validate_note',
            side_effect=ValueError('unsafe en release note'),
        ):
            with self.assertRaisesRegex(ValueError, 'unsafe en release note'):
                sign_ota_release.verify_global_manifest_release_notes(manifest, self.notes_path)

    def test_verify_cli_accepts_signed_manifest(self):
        manifest_path = self.root / 'manifest.json'
        manifest_path.write_text(
            json.dumps({'release_notes': VALID_NOTES['en']}, ensure_ascii=False) + '\n',
            encoding='utf-8',
        )
        with mock.patch.object(
            sys,
            'argv',
            [
                'sign_ota_release.py',
                '--verify-release-notes',
                '--manifest',
                str(manifest_path),
                '--release-notes',
                str(self.notes_path),
            ],
        ):
            sign_ota_release.main()


if __name__ == '__main__':
    unittest.main()
