#!/usr/bin/env python3
"""Download pinned NotoSansSC-Regular.otf into lib/EpdFont (gitignored source tree)."""

from __future__ import annotations

import hashlib
import sys
import urllib.request
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
PINS = ROOT / "lib/EpdFont/scripts/font-release-pins.yaml"
EPDFONT = ROOT / "lib/EpdFont"


def main() -> int:
    pins = yaml.safe_load(PINS.read_text())
    spec = pins["NotoSansSC"]["source"]
    dest = EPDFONT / spec["relative_path"]
    dest.parent.mkdir(parents=True, exist_ok=True)
    expected = spec["sha256"].lower()
    if dest.is_file():
        digest = hashlib.sha256(dest.read_bytes()).hexdigest()
        if digest == expected:
            print(f"NotoSansSC source already present at {dest}")
            return 0
        print(f"Removing stale NotoSansSC source (sha256 {digest})", file=sys.stderr)
        dest.unlink()
    print(f"Downloading NotoSansSC from {spec['url']}")
    urllib.request.urlretrieve(spec["url"], dest)
    digest = hashlib.sha256(dest.read_bytes()).hexdigest()
    if digest != expected:
        dest.unlink(missing_ok=True)
        print(f"::error::NotoSansSC sha256 mismatch: expected {expected}, got {digest}", file=sys.stderr)
        return 1
    print(f"Verified NotoSansSC sha256 {digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
