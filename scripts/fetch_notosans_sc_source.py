#!/usr/bin/env python3
"""Verify pinned NotoSansSC masters (offline; no network)."""

from __future__ import annotations

import hashlib
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
PINS = ROOT / "lib/EpdFont/scripts/font-release-pins.yaml"
PINNED = ROOT / "lib/EpdFont/scripts/pinned_sources"


def verify_file(path: Path, expected: str) -> None:
    if not path.is_file():
        raise SystemExit(f"::error::Missing pinned font: {path}")
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if digest != expected.lower():
        raise SystemExit(f"::error::{path.name} sha256 mismatch: expected {expected}, got {digest}")


def main() -> int:
    pins = yaml.safe_load(PINS.read_text())
    spec = pins["NotoSansSC"]
    for entry in spec["sources"]:
        dest = PINNED / entry["relative_path"]
        verify_file(dest, entry["sha256"])
    print("NotoSansSC pinned sources verified.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
