#!/usr/bin/env python3
"""Generate test/fixtures OTA signing known-answer fixture (test key only)."""

import base64
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PRIVATE = ROOT / 'test/fixtures/ota_test_private.pem'
JSON_OUT = ROOT / 'test/fixtures/ota_signing_known_answer.json'
HEADER_OUT = ROOT / 'test/fixtures/ota_signing_known_answer.generated.h'


def main():
    from cryptography.hazmat.primitives import hashes, serialization
    from cryptography.hazmat.primitives.asymmetric import ec
    from cryptography.hazmat.primitives.asymmetric.utils import Prehashed

    version = '1.6.0-waveshare-epaper-397-rc.99999'
    board = 'waveshare_epaper_397'
    size = 12345
    sha = '0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef'
    canonical = f'{version}|{board}|{size}|{sha.lower()}'.encode()
    digest = hashlib.sha256(canonical).digest()
    private_pem = PRIVATE.read_text()
    key = serialization.load_pem_private_key(private_pem.encode(), password=None)
    signature = key.sign(digest, ec.ECDSA(Prehashed(hashes.SHA256())))
    public_pem = subprocess.check_output(['openssl', 'ec', '-in', str(PRIVATE), '-pubout'], text=True).strip()

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
    JSON_OUT.write_text(json.dumps(fixture, indent=2) + '\n')

    HEADER_OUT.write_text(
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
    print(f'Wrote {JSON_OUT} and {HEADER_OUT}')


if __name__ == '__main__':
    main()
