#!/usr/bin/env python3
"""Generate an ECDSA P-256 OTA signing keypair and embed the public key in firmware."""

import argparse
import subprocess
import sys
from pathlib import Path

HEADER_PATH = Path('src/network/OtaSigningPublicKey.h')


def format_ota_signing_header(public_pem: str) -> str:
    lines = public_pem.strip().splitlines()
    body = '\n'.join(f'    "{line}\\n"' for line in lines)
    return f'''#pragma once

// ECDSA P-256 OTA signing public key (PEM). Regenerate with scripts/ota_keygen.py.
inline constexpr char OTA_SIGNING_PUBLIC_KEY_PEM[] =
{body};
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output-dir', type=Path, default=Path('dist/ota-keys'))
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    private_path = args.output_dir / 'ota_signing_private.pem'
    public_path = args.output_dir / 'ota_signing_public.pem'

    subprocess.check_call(
        ['openssl', 'ecparam', '-name', 'prime256v1', '-genkey', '-noout', '-out', str(private_path)]
    )
    subprocess.check_call(['openssl', 'ec', '-in', str(private_path), '-pubout', '-out', str(public_path)])

    public_pem = public_path.read_text()
    HEADER_PATH.write_text(format_ota_signing_header(public_pem))
    print(f'Wrote public key header to {HEADER_PATH}')
    print(f'Private key: {private_path}')
    print('Add the private key PEM to GitHub secret OTA_SIGNING_KEY (repo Settings -> Secrets).')
    return 0


if __name__ == '__main__':
    sys.exit(main())
