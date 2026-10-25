#!/usr/bin/env python3
"""Fail if tracked files contain private keys (PEM blocks, blocked extensions, or DER magic)."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

DER_SCAN_MAX_BYTES = 16 * 1024

# Block these extensions outright (case-insensitive); do not commit private-key blobs.
BLOCKED_PRIVATE_EXTENSIONS = frozenset({'.der', '.p12', '.pfx', '.pk8', '.ppk', '.key'})

# Matches -----BEGIN ... PRIVATE KEY----- (EC, RSA, ENCRYPTED, etc.), any case.
PRIVATE_KEY_BLOCK_RE = re.compile(
    r'-----BEGIN\s+(?:[A-Z0-9]+\s+)*PRIVATE\s+KEY-----',
    re.IGNORECASE,
)

# SEC1 ECPrivateKey (P-256) from `openssl ec -outform DER`.
SEC1_EC_PRIVATE_PREFIX = bytes.fromhex('30770201010420')

# PKCS#8 PrivateKeyInfo for P-256 from `openssl pkcs8 -topk8 -nocrypt -outform DER`.
PKCS8_EC_PRIVATE_PREFIX = bytes.fromhex('308187020100301306')

# PKCS#1 RSAPrivateKey: outer SEQUENCE (30 82) then version INTEGER 0 (02 01 00) at offset 4.
RSA_PKCS1_PRIVATE_HEADER = bytes.fromhex('3082')
RSA_PKCS1_VERSION_AT_OFFSET = 4
RSA_PKCS1_VERSION_BYTES = bytes.fromhex('020100')


def has_blocked_private_extension(rel_path: str) -> bool:
    return Path(rel_path).suffix.lower() in BLOCKED_PRIVATE_EXTENSIONS


def is_probably_text_file(path: Path) -> bool:
    try:
        sample = path.read_bytes()[:8192]
    except OSError:
        return False
    if b'\0' in sample:
        return False
    return True


def tracked_files(repo_root: Path | None = None) -> list[str]:
    root = repo_root or ROOT
    proc = subprocess.run(
        ['git', 'ls-files'],
        cwd=root,
        check=True,
        capture_output=True,
        text=True,
    )
    return [line.strip() for line in proc.stdout.splitlines() if line.strip()]


def file_contains_private_key_block(text: str) -> bool:
    return PRIVATE_KEY_BLOCK_RE.search(text) is not None


def looks_like_sec1_ec_private(data: bytes) -> bool:
    return len(data) >= len(SEC1_EC_PRIVATE_PREFIX) and data.startswith(SEC1_EC_PRIVATE_PREFIX)


def looks_like_pkcs8_ec_private(data: bytes) -> bool:
    return len(data) >= len(PKCS8_EC_PRIVATE_PREFIX) and data.startswith(PKCS8_EC_PRIVATE_PREFIX)


def looks_like_rsa_pkcs1_private(data: bytes) -> bool:
    if len(data) < RSA_PKCS1_VERSION_AT_OFFSET + len(RSA_PKCS1_VERSION_BYTES):
        return False
    if not data.startswith(RSA_PKCS1_PRIVATE_HEADER):
        return False
    return data[RSA_PKCS1_VERSION_AT_OFFSET : RSA_PKCS1_VERSION_AT_OFFSET + len(RSA_PKCS1_VERSION_BYTES)] == (
        RSA_PKCS1_VERSION_BYTES
    )


def file_has_der_private_magic(data: bytes) -> bool:
    return (
        looks_like_sec1_ec_private(data)
        or looks_like_pkcs8_ec_private(data)
        or looks_like_rsa_pkcs1_private(data)
    )


def scan_tracked_files(repo_root: Path | None = None) -> list[str]:
    root = repo_root or ROOT
    offenders: list[str] = []
    for rel in tracked_files(root):
        path = root / rel
        if not path.is_file():
            continue
        if has_blocked_private_extension(rel):
            offenders.append(rel)
            continue
        try:
            size = path.stat().st_size
        except OSError:
            continue
        if size <= DER_SCAN_MAX_BYTES:
            try:
                data = path.read_bytes()
            except OSError:
                continue
            if file_has_der_private_magic(data):
                offenders.append(rel)
                continue
        if not is_probably_text_file(path):
            continue
        try:
            text = path.read_text(encoding='utf-8', errors='surrogateescape')
        except OSError:
            continue
        if file_contains_private_key_block(text):
            offenders.append(rel)
    return offenders


def main() -> int:
    offenders = scan_tracked_files()
    if offenders:
        print('::error::Tracked files must not contain private keys:', file=sys.stderr)
        for rel in offenders:
            print(f'  {rel}', file=sys.stderr)
        return 1
    print('No tracked private keys (PEM blocks, blocked extensions, or DER magic).')
    return 0


if __name__ == '__main__':
    sys.exit(main())
