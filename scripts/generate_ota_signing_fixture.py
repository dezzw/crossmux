#!/usr/bin/env python3
"""Generate ephemeral OTA test signing fixtures (private key stays out of git)."""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_OUTPUT_DIR = ROOT / 'build' / 'test' / 'ota-fixtures'

OPENSSL_REQUIRED_MSG = 'openssl is required to generate OTA test fixtures'

PRIVATE_NAME = 'ota_test_private.pem'
TEST_PUBLIC_HEADER_NAME = 'OtaSigningTestPublicKey.h'
JSON_NAME = 'ota_signing_known_answer.json'
KNOWN_ANSWER_HEADER_NAME = 'ota_signing_known_answer.generated.h'

# Former committed test key (public and void; do not use for signing releases).
VOID_OTA_TEST_KEY_SPKI_SHA256 = (
    'b583a3d6edad2215e5def093eef52bfdff30b2ef242e9e0b03b45fd802282af4'
)


def require_openssl() -> None:
    if shutil.which('openssl') is None:
        print(f'::error::{OPENSSL_REQUIRED_MSG}', file=sys.stderr)
        raise SystemExit(1)
    try:
        subprocess.run(['openssl', 'version'], capture_output=True, check=True)
    except (OSError, subprocess.CalledProcessError):
        print(f'::error::{OPENSSL_REQUIRED_MSG}', file=sys.stderr)
        raise SystemExit(1)


def _run_openssl(args: list[str], **kwargs) -> subprocess.CompletedProcess:
    require_openssl()
    try:
        return subprocess.run(['openssl', *args], check=True, **kwargs)
    except FileNotFoundError:
        print(f'::error::{OPENSSL_REQUIRED_MSG}', file=sys.stderr)
        raise SystemExit(1) from None
    except subprocess.CalledProcessError as exc:
        print(f'::error::{OPENSSL_REQUIRED_MSG}', file=sys.stderr)
        raise SystemExit(exc.returncode) from exc


def generate_ec_private_key_pem(private_path: Path) -> None:
    private_path.parent.mkdir(parents=True, exist_ok=True)
    _run_openssl(
        ['ecparam', '-name', 'prime256v1', '-genkey', '-noout', '-out', str(private_path)],
    )


def public_pem_from_private_path(private_path: Path) -> str:
    proc = _run_openssl(['ec', '-in', str(private_path), '-pubout'], capture_output=True, text=True)
    return proc.stdout.strip()


def format_test_public_key_header(public_pem: str) -> str:
    lines = public_pem.strip().splitlines()
    body = '\n'.join(f'    "{line}\\n"' for line in lines)
    return (
        '#pragma once\n\n'
        '// Ephemeral OTA test public key (host tests only). Regenerate with '
        'scripts/generate_ota_signing_fixture.py.\n'
        'inline constexpr char OTA_SIGNING_TEST_PUBLIC_KEY_PEM[] =\n'
        f'{body};\n'
    )


def write_known_answer_fixtures(private_path: Path, output_dir: Path) -> None:
    from cryptography.hazmat.primitives import hashes, serialization
    from cryptography.hazmat.primitives.asymmetric import ec
    from cryptography.hazmat.primitives.asymmetric.utils import Prehashed

    version = '1.6.0-waveshare-epaper-397-rc.99999'
    board = 'waveshare_epaper_397'
    size = 12345
    sha = '0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef'
    canonical = f'{version}|{board}|{size}|{sha.lower()}'.encode()
    digest = hashlib.sha256(canonical).digest()
    private_pem = private_path.read_text()
    key = serialization.load_pem_private_key(private_pem.encode(), password=None)
    signature = key.sign(digest, ec.ECDSA(Prehashed(hashes.SHA256())))
    public_pem = public_pem_from_private_path(private_path)

    fixture = {
        'version': version,
        'boardTag': board,
        'firmwareSize': size,
        'sha256Hex': sha,
        'canonical': canonical.decode(),
        'digestHex': digest.hex(),
        'signatureBase64': base64.b64encode(signature).decode(),
        'signatureHex': signature.hex(),
        'publicKeyPem': public_pem,
    }
    json_out = output_dir / JSON_NAME
    json_out.write_text(json.dumps(fixture, indent=2) + '\n')

    header_out = output_dir / KNOWN_ANSWER_HEADER_NAME
    header_out.write_text(
        '#pragma once\n'
        f'inline constexpr char OTA_KNOWN_VERSION[] = "{version}";\n'
        f'inline constexpr char OTA_KNOWN_BOARD[] = "{board}";\n'
        f'inline constexpr size_t OTA_KNOWN_FIRMWARE_SIZE = {size};\n'
        f'inline constexpr char OTA_KNOWN_SHA256[] = "{sha}";\n'
        f'inline constexpr char OTA_KNOWN_DIGEST_HEX[] = "{digest.hex()}";\n'
        f'inline constexpr char OTA_KNOWN_SIGNATURE_HEX[] = "{signature.hex()}";\n'
        f'inline constexpr char OTA_KNOWN_PUBLIC_KEY_PEM[] =\n'
        + ''.join(f'    "{line}\\n"\n' for line in public_pem.splitlines())
        + ';\n'
    )


def generate_all(output_dir: Path) -> Path:
    """Create a fresh test keypair and known-answer fixtures under output_dir."""
    require_openssl()
    output_dir = output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    private_path = output_dir / PRIVATE_NAME
    generate_ec_private_key_pem(private_path)
    public_pem = public_pem_from_private_path(private_path)
    (output_dir / TEST_PUBLIC_HEADER_NAME).write_text(format_test_public_key_header(public_pem))
    write_known_answer_fixtures(private_path, output_dir)
    return output_dir


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        '--output-dir',
        type=Path,
        default=DEFAULT_OUTPUT_DIR,
        help=f'Directory for generated fixtures (default: {DEFAULT_OUTPUT_DIR})',
    )
    args = parser.parse_args()
    out = generate_all(args.output_dir)
    print(f'Wrote OTA test fixtures to {out}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
