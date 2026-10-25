#!/usr/bin/env python3
"""Verify OTA signing known-answer fixture with cryptography (Prehashed SHA256)."""

import base64
import hashlib
import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))

import ota_test_fixtures  # noqa: E402


class OtaSigningCrossTest(unittest.TestCase):
    def test_known_answer_verifies(self):
        from cryptography.exceptions import InvalidSignature
        from cryptography.hazmat.primitives import hashes, serialization
        from cryptography.hazmat.primitives.asymmetric import ec
        from cryptography.hazmat.primitives.asymmetric.utils import Prehashed

        fixture_path = ota_test_fixtures.known_answer_json(ota_test_fixtures.ensure_generated())
        fixture = json.loads(fixture_path.read_text())
        canonical = (
            f"{fixture['version']}|{fixture['boardTag']}|{fixture['firmwareSize']}|"
            f"{fixture['sha256Hex'].lower()}"
        ).encode()
        digest = hashlib.sha256(canonical).digest()
        self.assertEqual(digest.hex(), fixture['digestHex'])

        public_key = serialization.load_pem_public_key(fixture['publicKeyPem'].encode())
        self.assertIsInstance(public_key, ec.EllipticCurvePublicKey)
        signature = base64.b64decode(fixture['signatureBase64'])
        try:
            public_key.verify(signature, digest, ec.ECDSA(Prehashed(hashes.SHA256())))
        except InvalidSignature as exc:
            self.fail(f'signature verify failed: {exc}')


if __name__ == '__main__':
    unittest.main()
