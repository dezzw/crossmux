#!/usr/bin/env python3
"""Ensure OTA_SIGNING_KEY matches embedded public key and is not the test fixture."""

import hashlib
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HEADER = ROOT / 'src/network/OtaSigningPublicKey.h'

from generate_ota_signing_fixture import VOID_OTA_TEST_KEY_SPKI_SHA256  # noqa: E402


def spki_fingerprint_pem(pem: str) -> str:
    with subprocess.Popen(
        ['openssl', 'pkey', '-pubin', '-outform', 'DER'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=False,
    ) as proc:
        out, err = proc.communicate(pem.encode())
        if proc.returncode != 0:
            raise SystemExit(err.decode() or 'openssl pkey failed')
    return hashlib.sha256(out).hexdigest()


def decode_c_string_escapes(s: str) -> str:
    """Decode C string literal escapes (\\n, \\r, \\t, \\", \\\\) for embedded PEM."""
    out: list[str] = []
    i = 0
    while i < len(s):
        if s[i] == '\\' and i + 1 < len(s):
            esc = s[i + 1]
            if esc == 'n':
                out.append('\n')
                i += 2
                continue
            if esc == 'r':
                out.append('\r')
                i += 2
                continue
            if esc == 't':
                out.append('\t')
                i += 2
                continue
            if esc == '"':
                out.append('"')
                i += 2
                continue
            if esc == '\\':
                out.append('\\')
                i += 2
                continue
        out.append(s[i])
        i += 1
    return ''.join(out)


def read_embedded_public_pem() -> str:
    text = HEADER.read_text()
    match = re.search(r'inline constexpr char OTA_SIGNING_PUBLIC_KEY_PEM\[\] =\s*(.*?);', text, re.S)
    if not match:
        raise SystemExit(f'could not parse public key from {HEADER}')
    body = match.group(1)
    lines = re.findall(r'"([^"]*)"', body)
    return decode_c_string_escapes(''.join(lines))


def void_test_key_fingerprint() -> str:
    """SPKI SHA-256 of the former committed test public key (public and void)."""
    return VOID_OTA_TEST_KEY_SPKI_SHA256


def main():
    private_pem = os.environ.get('OTA_SIGNING_KEY', '').strip()
    if not private_pem:
        print('::error::OTA_SIGNING_KEY secret is required', file=sys.stderr)
        return 1

    derived_public = subprocess.check_output(
        ['openssl', 'ec', '-pubout'],
        input=private_pem,
        text=True,
    )
    embedded = read_embedded_public_pem()
    if derived_public.strip() != embedded.strip():
        print('::error::OTA_SIGNING_KEY does not match OtaSigningPublicKey.h', file=sys.stderr)
        return 1

    embedded_fp = spki_fingerprint_pem(embedded)
    if embedded_fp == void_test_key_fingerprint():
        print('::error::Production OTA public key must not be the void test fixture key', file=sys.stderr)
        return 1

    print(f'OTA signing public key fingerprint ok ({embedded_fp[:16]}…)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
