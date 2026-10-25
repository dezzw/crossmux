#!/usr/bin/env python3
"""Fail if any tracked text file contains a PEM private-key block."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Matches -----BEGIN ... PRIVATE KEY----- (EC, RSA, ENCRYPTED, etc.), any case.
PRIVATE_KEY_BLOCK_RE = re.compile(
    r'-----BEGIN\s+(?:[A-Z0-9]+\s+)*PRIVATE\s+KEY-----',
    re.IGNORECASE,
)


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


def scan_tracked_files(repo_root: Path | None = None) -> list[str]:
    root = repo_root or ROOT
    offenders: list[str] = []
    for rel in tracked_files(root):
        path = root / rel
        if not path.is_file() or not is_probably_text_file(path):
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
        print('::error::Tracked files must not contain PEM private-key blocks:', file=sys.stderr)
        for rel in offenders:
            print(f'  {rel}', file=sys.stderr)
        return 1
    print('No tracked PEM private-key blocks.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
