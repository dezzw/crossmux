"""Ensure CrossMux wolfSSL pre-build patch enables SHA-384/512 for device TLS."""

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class WolfSslSha384PatchTest(unittest.TestCase):
    def test_patch_script_enables_sha384_sha512_and_sp384(self):
        text = (ROOT / "scripts/patch_wolfssl.py").read_text()
        for needle in ("WOLFSSL_SHA384", "WOLFSSL_SHA512", "WOLFSSL_SP_384"):
            self.assertIn(needle, text)

    def test_secure_client_patch_checks_ca_load(self):
        source = (ROOT / "scripts/patch_secure_client.py").read_text()
        self.assertIn("wolfSSL_CTX_load_verify_buffer", source)
        self.assertIn("loadRc != WOLFSSL_SUCCESS", source)
        self.assertIn("roots_in_pem", source)
        self.assertIn("lastWolfErrorCode", source)


if __name__ == "__main__":
    unittest.main()
