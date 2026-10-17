#!/usr/bin/env python3
"""Helpers for signed OTA global manifests (sign_ota_release.py output)."""

from __future__ import annotations

import json
import sys
from pathlib import Path


def firmware_url_from_manifest(manifest: dict) -> str:
    firmware = manifest.get("firmware")
    if not isinstance(firmware, dict):
        raise ValueError("manifest missing firmware object")
    url = str(firmware.get("url", "")).strip()
    if not url:
        raise ValueError("manifest firmware.url is empty")
    return url


def firmware_url_from_path(path: Path) -> str:
    manifest = json.loads(path.read_text(encoding="utf-8"))
    return firmware_url_from_manifest(manifest)


def main() -> int:
    if len(sys.argv) != 3 or sys.argv[1] != "url":
        print("usage: ota_manifest.py url <manifest.json>", file=sys.stderr)
        return 2
    try:
        print(firmware_url_from_path(Path(sys.argv[2])))
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        print(f"::error::{exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
