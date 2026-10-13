#!/usr/bin/env python3
"""Sign firmware SHA-256 and write device OTA manifest fields."""

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


def sign_digest(private_pem: str, digest: bytes) -> bytes:
    from cryptography.hazmat.primitives import hashes, serialization
    from cryptography.hazmat.primitives.asymmetric import ec

    key = serialization.load_pem_private_key(private_pem.encode(), password=None)
    if not isinstance(key, ec.EllipticCurvePrivateKey):
        raise SystemExit('OTA_SIGNING_KEY must be an EC private key')
    return key.sign(digest, ec.ECDSA(hashes.SHA256()))


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

    digest = bytes.fromhex(sha256_file(firmware))
    signature = sign_digest(private_pem, digest)

    manifest = json.loads(args.manifest.read_text())
    manifest['firmware'] = {
        'url': args.download_url,
        'size': firmware.stat().st_size,
        'sha256': digest.hex(),
        'signature': base64.b64encode(signature).decode('ascii'),
    }
    if 'signature' in manifest:
        manifest['signature'] = manifest['firmware']['signature']
    args.output.write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Signed manifest written to {args.output}')


if __name__ == '__main__':
    main()
