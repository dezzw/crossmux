#!/usr/bin/env python3
"""OTA test fixtures must work with any freshly generated key (not the old committed key)."""

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))

import generate_ota_signing_fixture
import ota_test_fixtures


class OtaTestFixturesEphemeralTest(unittest.TestCase):
    def _verify_known_answer(self, fixture_dir: Path) -> None:
        from cryptography.exceptions import InvalidSignature
        from cryptography.hazmat.primitives import hashes, serialization
        from cryptography.hazmat.primitives.asymmetric import ec
        from cryptography.hazmat.primitives.asymmetric.utils import Prehashed

        fixture_path = fixture_dir / generate_ota_signing_fixture.JSON_NAME
        fixture = json.loads(fixture_path.read_text())
        digest = bytes.fromhex(fixture['digestHex'])
        public_key = serialization.load_pem_public_key(fixture['publicKeyPem'].encode())
        signature = bytes.fromhex(fixture['signatureHex'])
        try:
            public_key.verify(signature, digest, ec.ECDSA(Prehashed(hashes.SHA256())))
        except InvalidSignature as exc:
            self.fail(str(exc))

        import hashlib

        public_pem = subprocess.check_output(
            ['openssl', 'ec', '-in', str(fixture_dir / generate_ota_signing_fixture.PRIVATE_NAME), '-pubout'],
            text=True,
        )
        der = subprocess.check_output(
            ['openssl', 'pkey', '-pubin', '-outform', 'DER'],
            input=public_pem.encode(),
        )
        spki = hashlib.sha256(der).hexdigest()
        self.assertNotEqual(spki, generate_ota_signing_fixture.VOID_OTA_TEST_KEY_SPKI_SHA256)

    def test_two_independent_generated_keys(self):
        for _ in range(2):
            with tempfile.TemporaryDirectory() as tmp:
                out = Path(tmp)
                generate_ota_signing_fixture.generate_all(out)
                self._verify_known_answer(out)

    def test_ensure_generated_default_dir(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / 'fixtures'
            pem = ota_test_fixtures.ensure_generated(out)
            self.assertEqual(pem, out.resolve())
            self._verify_known_answer(out)


if __name__ == '__main__':
    unittest.main()
