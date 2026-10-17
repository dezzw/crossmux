#!/usr/bin/env python3
"""Pinned core font codepoint coverage hashes and legacy Noto bitmap list."""

from __future__ import annotations

import hashlib
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
PINS_PATH = ROOT / "lib/EpdFont/scripts/core_font_coverage_pins.yaml"
SCRIPTS = ROOT / "scripts"

STYLE_CODEPOINT_HASH = "sha256"


def codepoint_set_hash(codepoints: list[int] | set[int]) -> str:
    payload = ",".join(str(cp) for cp in sorted(set(codepoints)))
    return hashlib.sha256(payload.encode("utf-8")).hexdigest()


def load_pins() -> dict:
    return yaml.safe_load(PINS_PATH.read_text(encoding="utf-8"))


def verify_legacy_bitmap_file(path: Path, expected_sha256: str) -> None:
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if digest != expected_sha256.lower():
        raise ValueError(f"{path.name}: sha256 mismatch (expected {expected_sha256}, got {digest})")


def legacy_bitmap_codepoints(path: Path) -> set[int]:
    allowed: set[int] = set()
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        if all(c in "0123456789abcdefABCDEF" for c in line) and len(line) <= 8:
            allowed.add(int(line, 16))
            continue
        for ch in line:
            allowed.add(ord(ch))
    return allowed
