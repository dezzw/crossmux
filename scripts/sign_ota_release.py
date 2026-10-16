#!/usr/bin/env python3
"""Sign OTA manifest (canonical payload) and write device manifest fields."""

import argparse
import base64
import hashlib
import json
import os
import sys
from pathlib import Path


def load_private_key_pem() -> str:
    raw = os.environ.get('OTA_SIGNING_KEY', '').strip()
    if not raw:
        raise SystemExit('OTA_SIGNING_KEY is not set')
    return raw


def canonical_payload(version: str, board_tag: str, size: int, sha256_hex: str) -> bytes:
    return f'{version}|{board_tag}|{size}|{sha256_hex.lower()}'.encode('utf-8')


def sign_digest(private_pem: str, digest: bytes) -> bytes:
    from cryptography.hazmat.primitives import hashes, serialization
    from cryptography.hazmat.primitives.asymmetric import ec
    from cryptography.hazmat.primitives.asymmetric.utils import Prehashed

    key = serialization.load_pem_private_key(private_pem.encode(), password=None)
    if not isinstance(key, ec.EllipticCurvePrivateKey):
        raise SystemExit('OTA_SIGNING_KEY must be an EC private key')
    return key.sign(digest, ec.ECDSA(Prehashed(hashes.SHA256())))


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--firmware', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--download-url', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()

    private_pem = load_private_key_pem()
    firmware = args.firmware
    if not firmware.is_file():
        raise SystemExit(f'missing firmware: {firmware}')

    manifest = json.loads(args.manifest.read_text())
    version = manifest.get('version')
    board_tag = manifest.get('boardTag')
    if not version or not board_tag:
        raise SystemExit('manifest must include version and boardTag before signing')

    fw_sha256 = sha256_file(firmware)
    fw_size = firmware.stat().st_size
    payload = canonical_payload(version, board_tag, fw_size, fw_sha256)
    signing_digest = hashlib.sha256(payload).digest()
    signature = sign_digest(private_pem, signing_digest)

    manifest['firmware'] = {
        'url': args.download_url,
        'size': fw_size,
        'sha256': fw_sha256,
        'signature': base64.b64encode(signature).decode('ascii'),
    }
    manifest['signature'] = manifest['firmware']['signature']
    args.output.write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Signed manifest written to {args.output}')


if __name__ == '__main__':
    main()
