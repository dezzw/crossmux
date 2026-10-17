#!/usr/bin/env python3
"""Download pinned NotoSansSC masters into lib/EpdFont (gitignored source tree)."""

from __future__ import annotations

import hashlib
import sys
import urllib.request
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
PINS = ROOT / "lib/EpdFont/scripts/font-release-pins.yaml"
EPDFONT = ROOT / "lib/EpdFont"


def verify_or_fetch(url: str, dest: Path, expected: str) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    expected = expected.lower()
    if dest.is_file():
        digest = hashlib.sha256(dest.read_bytes()).hexdigest()
        if digest == expected:
            print(f"Already present: {dest.name} ({digest[:12]}…)")
            return
        print(f"Removing stale {dest.name} (sha256 {digest})", file=sys.stderr)
        dest.unlink()
    print(f"Downloading {dest.name} from {url}")
    urllib.request.urlretrieve(url, dest)
    digest = hashlib.sha256(dest.read_bytes()).hexdigest()
    if digest != expected:
        dest.unlink(missing_ok=True)
        raise SystemExit(f"::error::{dest.name} sha256 mismatch: expected {expected}, got {digest}")


def main() -> int:
    pins = yaml.safe_load(PINS.read_text())
    spec = pins["NotoSansSC"]
    for entry in spec["sources"]:
        dest = EPDFONT / entry["relative_path"]
        verify_or_fetch(entry["url"], dest, entry["sha256"])
    print("NotoSansSC sources verified.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
