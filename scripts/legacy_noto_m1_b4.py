#!/usr/bin/env python3
"""Pinned crossmux.com m1-b4 NotoSansSC_12.cpfont URL and integrity check."""

from __future__ import annotations

import hashlib
import urllib.request
from pathlib import Path

LEGACY_NOTO_URL = (
    "https://assets.crossmux.cn/fonts/m1-b4/releases/"
    "71141b4aeb4a3832824730fef73045f0d1c7f19c/NotoSansSC_12.cpfont"
)
LEGACY_NOTO_SHA256 = "8797b8b3c74b36184add5a378a4bc813f447ff462a3164ade97a046ccc4875bb"


def verify_legacy_noto(path: Path) -> None:
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if digest != LEGACY_NOTO_SHA256:
        raise OSError(f"{path.name}: sha256 mismatch (expected {LEGACY_NOTO_SHA256}, got {digest})")


def download_legacy_noto(dest: Path) -> None:
    if dest.is_file():
        try:
            verify_legacy_noto(dest)
            return
        except OSError:
            dest.unlink(missing_ok=True)
    dest.parent.mkdir(parents=True, exist_ok=True)
    urllib.request.urlretrieve(LEGACY_NOTO_URL, dest)
    verify_legacy_noto(dest)
