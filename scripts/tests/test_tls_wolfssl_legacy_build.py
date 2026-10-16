"""Build tls_chain_verify with device/legacy wolfSSL flags; offline chain + live host checks."""

import socket
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BUILD_SHA = ROOT / "build/tls_chain_verify_sha384"
BUILD_LEGACY = ROOT / "build/tls_chain_verify_legacy"
BUILD_NO_ALT = ROOT / "build/tls_chain_verify_no_alt"


def _online() -> bool:
    try:
        with socket.create_connection(("github.com", 443), timeout=5):
            return True
    except OSError:
        return False


def _bundle_without_pinned_globalsign() -> Path:
    """Device bundle includes legacy GlobalSign Root CA; omit it to test ALT chain selection."""
    import re
    import tempfile

    pem = (ROOT / "src/network/TlsCaBundle.pem").read_text()
    certs = re.findall(r"-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----", pem, re.DOTALL)
    kept = []
    for cert in certs:
        with tempfile.NamedTemporaryFile("w", suffix=".pem", delete=False) as handle:
            handle.write(cert)
            path = handle.name
        subject = subprocess.check_output(["openssl", "x509", "-in", path, "-noout", "-subject"], text=True)
        Path(path).unlink(missing_ok=True)
        if "GlobalSign Root CA" in subject and "R3" not in subject and "R6" not in subject:
            continue
        kept.append(cert.strip())
    out = ROOT / "build/tls_crossmux_test_bundle_no_gs_root.pem"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n\n".join(kept) + "\n")
    return out


def _configure_build(build_dir: Path, sha384: bool, alt_chains: bool = True) -> None:
    if build_dir.exists():
        import shutil

        shutil.rmtree(build_dir)
    build_dir.mkdir(parents=True, exist_ok=True)
    sha_flag = "ON" if sha384 else "OFF"
    alt_flag = "ON" if alt_chains else "OFF"
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
            f"-DTLS_WOLFSSL_SHA384={sha_flag}",
            f"-DTLS_WOLFSSL_ALT_CHAINS={alt_flag}",
        ]
    )
    subprocess.check_call(
        ["cmake", "--build", str(build_dir), "--target", "TlsCaChainVerifyTest", "TlsHostVerify", "-j"]
    )


class TlsWolfSslOfflineTests(unittest.TestCase):
    def test_github_chain_passes_with_device_sha384_config(self):
        _configure_build(BUILD_SHA, sha384=True)
        subprocess.check_call(
            [
                str(BUILD_SHA / "tls_chain_verify/TlsCaChainVerifyTest"),
                "--gtest_filter=TlsCaChainVerifyTest.GithubSectigoE36Intermediate",
            ]
        )

    def test_github_chain_fails_without_sha384(self):
        _configure_build(BUILD_LEGACY, sha384=False)
        proc = subprocess.run(
            [
                str(BUILD_LEGACY / "tls_chain_verify/TlsCaChainVerifyTest"),
                "--gtest_filter=TlsCaChainVerifyTest.GithubSectigoE36Intermediate",
            ],
            check=False,
        )
        self.assertNotEqual(proc.returncode, 0, "expected GitHub intermediate verify to fail without SHA-384")

    def test_crossmux_pem_chain_documents_asn_no_signer_offline(self):
        _configure_build(BUILD_SHA, sha384=True)
        subprocess.check_call(
            [
                str(BUILD_SHA / "tls_chain_verify/TlsCaChainVerifyTest"),
                "--gtest_filter=TlsCaChainVerifyTest.CrossmuxServedChainPemNeedsAltChainsAtHandshake",
            ]
        )


class TlsWolfSslNetworkTests(unittest.TestCase):
    @unittest.skipUnless(_online(), "network required for live TLS host verification")
    def test_crossmux_live_host_fails_without_alt_cert_chains(self):
        _configure_build(BUILD_NO_ALT, sha384=True, alt_chains=False)
        bundle = _bundle_without_pinned_globalsign()
        proc = subprocess.run(
            [
                str(BUILD_NO_ALT / "tls_chain_verify/TlsHostVerify"),
                "crossmux.com",
                str(bundle),
            ],
            check=False,
            capture_output=True,
            text=True,
        )
        self.assertNotEqual(proc.returncode, 0, proc.stdout + proc.stderr)

    @unittest.skipUnless(_online(), "network required for live TLS host verification")
    def test_crossmux_live_host_passes_with_device_config(self):
        _configure_build(BUILD_SHA, sha384=True, alt_chains=True)
        bundle = ROOT / "src/network/TlsCaBundle.pem"
        subprocess.check_call(
            [str(BUILD_SHA / "tls_chain_verify/TlsHostVerify"), "crossmux.com", str(bundle)],
        )


if __name__ == "__main__":
    unittest.main()
