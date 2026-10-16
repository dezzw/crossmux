"""Build tls_chain_verify twice: SHA-384 enabled (pass) and disabled (fail on GitHub chain)."""

import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BUILD_SHA = ROOT / "build/tls_chain_verify_sha384"
BUILD_LEGACY = ROOT / "build/tls_chain_verify_legacy"


class TlsWolfSslLegacyBuildTest(unittest.TestCase):
    def _configure_build(self, build_dir: Path, sha384: bool) -> None:
        if build_dir.exists():
            import shutil

            shutil.rmtree(build_dir)
        build_dir.mkdir(parents=True, exist_ok=True)
        flag = "ON" if sha384 else "OFF"
        subprocess.check_call(
            [
                "cmake",
                "-S",
                str(ROOT / "test"),
                "-B",
                str(build_dir),
                "-G",
                "Ninja",
                "-DCMAKE_BUILD_TYPE=Release",
                f"-DTLS_WOLFSSL_SHA384={flag}",
            ]
        )
        subprocess.check_call(["cmake", "--build", str(build_dir), "--target", "TlsCaChainVerifyTest", "-j"])

    def test_github_chain_passes_with_device_sha384_config(self):
        self._configure_build(BUILD_SHA, sha384=True)
        subprocess.check_call(
            [
                str(BUILD_SHA / "tls_chain_verify/TlsCaChainVerifyTest"),
                "--gtest_filter=TlsCaChainVerifyTest.GithubSectigoE36Intermediate",
            ]
        )

    def test_github_chain_fails_without_sha384(self):
        self._configure_build(BUILD_LEGACY, sha384=False)
        proc = subprocess.run(
            [
                str(BUILD_LEGACY / "tls_chain_verify/TlsCaChainVerifyTest"),
                "--gtest_filter=TlsCaChainVerifyTest.GithubSectigoE36Intermediate",
            ],
            check=False,
        )
        self.assertNotEqual(proc.returncode, 0, "expected GitHub intermediate verify to fail without SHA-384")


if __name__ == "__main__":
    unittest.main()
