#!/usr/bin/env python3
"""Sign OTA manifest (canonical payload) and write device manifest fields."""

import argparse
import base64
import hashlib
import json
import os
import sys
from pathlib import Path

from generate_ota_notes import extract_notes

FLAVOR_NOTE_LANGUAGE = {'global': 'en', 'zh-CN': 'zh'}


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


def apply_release_notes(manifest: dict, notes_path: Path) -> None:
    if not notes_path.is_file():
        raise SystemExit(f'missing release notes: {notes_path}')
    try:
        notes = extract_notes(notes_path.read_text(encoding='utf-8'))
    except ValueError as error:
        raise SystemExit(str(error)) from error
    flavor = manifest.get('flavor')
    language = FLAVOR_NOTE_LANGUAGE.get(flavor)
    if language is None:
        raise SystemExit(f'unknown manifest flavor for release notes: {flavor!r}')
    manifest['release_notes'] = list(notes[language])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--firmware', type=Path)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--download-url')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--release-notes', type=Path)
    parser.add_argument(
        '--embed-notes-only',
        action='store_true',
        help='write release_notes into the manifest without signing firmware',
    )
    args = parser.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding='utf-8'))
    if args.release_notes:
        apply_release_notes(manifest, args.release_notes)

    if args.embed_notes_only:
        output = args.output or args.manifest
        output.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
        print(f'Release notes written to {output}')
        return

    if not args.firmware or not args.download_url or not args.output:
        raise SystemExit('--firmware, --download-url, and --output are required unless --embed-notes-only')
    private_pem = load_private_key_pem()
    firmware = args.firmware
    if not firmware.is_file():
        raise SystemExit(f'missing firmware: {firmware}')

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
    args.output.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(f'Signed manifest written to {args.output}')


if __name__ == '__main__':
    main()
