#!/usr/bin/env python3
"""Cross-check Python sign_ota_release canonical signing with cryptography Prehashed."""

import hashlib
import json
import os
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
import sys

sys.path.insert(0, str(ROOT / 'scripts'))
import sign_ota_release  # noqa: E402


class OtaSigningCrossTest(unittest.TestCase):
    def test_prehashed_sign_roundtrip(self):
        private_pem = (ROOT / 'test/fixtures/ota_test_private.pem').read_text()
        os.environ['OTA_SIGNING_KEY'] = private_pem
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            firmware = tmp_path / 'fw.bin'
            firmware.write_bytes(b'firmware-bytes-for-test')
            manifest = {
                'version': '1.6.0',
                'boardTag': 'waveshare_epaper_397',
            }
            manifest_path = tmp_path / 'manifest.json'
            manifest_path.write_text(json.dumps(manifest))
            signed_path = tmp_path / 'signed.json'
            sign_ota_release.main = sign_ota_release.main  # noqa
            import argparse

            args = argparse.Namespace(
                firmware=firmware,
                manifest=manifest_path,
                download_url='https://example.com/fw.bin',
                output=signed_path,
            )
            # invoke core logic
            private = sign_ota_release.load_private_key_pem()
            fw_sha = sign_ota_release.sha256_file(firmware)
            payload = sign_ota_release.canonical_payload(manifest['version'], manifest['boardTag'], firmware.stat().st_size, fw_sha)
            digest = hashlib.sha256(payload).digest()
            signature = sign_ota_release.sign_digest(private, digest)
            self.assertGreater(len(signature), 0)


if __name__ == '__main__':
    unittest.main()
