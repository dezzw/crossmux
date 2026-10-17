#!/usr/bin/env python3
"""Download pinned core font masters (sha256 from font-release-pins.yaml)."""

from __future__ import annotations

import hashlib
import sys
import urllib.request
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
PINS = ROOT / "lib/EpdFont/scripts/font-release-pins.yaml"
SD_FONTS = ROOT / "lib/EpdFont/scripts/sd-fonts.yaml"
EPDFONT = ROOT / "lib/EpdFont"
DOWNLOAD_DIR = EPDFONT / "scripts/downloaded_fonts"
NOTO_DIR = EPDFONT / "builtinFonts/source/NotoSansSC"


def verify_or_fetch(url: str, dest: Path, expected_sha256: str) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    expected = expected_sha256.lower()
    if dest.is_file():
        digest = hashlib.sha256(dest.read_bytes()).hexdigest()
        if digest == expected:
            print(f"Already present: {dest.relative_to(ROOT)} ({digest[:12]}…)")
            return
        print(f"Removing stale {dest.name} (sha256 {digest})", file=sys.stderr)
        dest.unlink()
    print(f"Downloading {dest.relative_to(ROOT)}")
    urllib.request.urlretrieve(url, dest)
    digest = hashlib.sha256(dest.read_bytes()).hexdigest()
    if digest != expected:
        dest.unlink(missing_ok=True)
        raise SystemExit(f"::error::{dest.name} sha256 mismatch: expected {expected}, got {digest}")


def fetch_noto_sans_sc(pins: dict) -> None:
    spec = pins["NotoSansSC"]
    for entry in spec["sources"]:
        name = Path(entry["relative_path"]).name
        dest = NOTO_DIR / name
        verify_or_fetch(entry["url"], dest, entry["sha256"])


def fetch_from_sd_fonts(pins: dict) -> None:
    config = yaml.safe_load(SD_FONTS.read_text(encoding="utf-8"))
    for family in config.get("families", []):
        name = family["name"]
        if name == "NotoSansSC":
            continue
        family_pins = pins.get(name, {})
        source_pins = family_pins.get("sources") or {}
        for style_name, style_spec in family.get("styles", {}).items():
            url = style_spec.get("url")
            if not url:
                continue
            pin = source_pins.get(style_name)
            if not pin:
                raise SystemExit(f"::error::No sha256 pin for {name}/{style_name} in font-release-pins.yaml")
            filename = url.rsplit("/", 1)[-1]
            dest = DOWNLOAD_DIR / name / filename
            verify_or_fetch(url, dest, pin["sha256"])


def main() -> int:
    pins = yaml.safe_load(PINS.read_text(encoding="utf-8"))
    fetch_noto_sans_sc(pins)
    fetch_from_sd_fonts(pins)
    print("Core font sources verified.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
