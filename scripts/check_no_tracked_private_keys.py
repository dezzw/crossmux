#!/usr/bin/env python3
"""Fail if git tracks any PEM file containing a private key block."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MARKER = 'PRIVATE KEY'


def tracked_pem_files() -> list[str]:
    proc = subprocess.run(
        ['git', 'ls-files', '*.pem'],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    return [line.strip() for line in proc.stdout.splitlines() if line.strip()]


def main() -> int:
    offenders: list[str] = []
    for rel in tracked_pem_files():
        path = ROOT / rel
        if not path.is_file():
            continue
        if MARKER in path.read_text():
            offenders.append(rel)
    if offenders:
        print('::error::Tracked PEM files must not contain private keys:', file=sys.stderr)
        for rel in offenders:
            print(f'  {rel}', file=sys.stderr)
        return 1
    print('No tracked private-key PEM files.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
