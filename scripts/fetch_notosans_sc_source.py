#!/usr/bin/env python3
"""Download pinned NotoSansSC masters (compat wrapper)."""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "scripts"))

from fetch_core_font_sources import fetch_noto_sans_sc, PINS  # noqa: E402

import yaml  # noqa: E402


def main() -> int:
    pins = yaml.safe_load(PINS.read_text(encoding="utf-8"))
    fetch_noto_sans_sc(pins)
    print("NotoSansSC sources verified.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
