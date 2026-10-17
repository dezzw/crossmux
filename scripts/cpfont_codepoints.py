#!/usr/bin/env python3
"""Extract codepoint coverage from v4 .cpfont files."""

from __future__ import annotations

import struct
from pathlib import Path

STYLE_TOC = struct.Struct("<B3xIIBhhHHBBBI4x")
GLYPH = struct.Struct("<BBHhhH2xI")


def cpfont_codepoints(path: Path, *, require_bitmap: bool = False) -> list[int]:
    data = path.read_bytes()
    if data[:6] != b"CPFONT":
        raise ValueError(f"not a CPFONT file: {path}")
    style_count = data[12]
    toc_off = 32
    style_id, interval_count, glyph_count, *_rest, data_offset = STYLE_TOC.unpack_from(data, toc_off)
    del style_id, _rest

    p = data_offset
    intervals: list[tuple[int, int]] = []
    for _ in range(interval_count):
        first, last, _offset = struct.unpack_from("<III", data, p)
        intervals.append((first, last))
        p += 12

    glyphs_off = p
    codepoints: list[int] = []
    gi = 0
    for first, last in intervals:
        for cp in range(first, last + 1):
            if gi >= glyph_count:
                break
            _w, _h, _ax, _l, _t, data_length, _doff = GLYPH.unpack_from(data, glyphs_off + gi * 16)
            if not require_bitmap or data_length > 0:
                codepoints.append(cp)
            gi += 1
    return codepoints


def cpfont_glyph_stats(path: Path) -> tuple[int, int, int]:
    data = path.read_bytes()
    style_count = data[12]
    max_glyphs = 0
    for i in range(style_count):
        _sid, _ic, gc, *_ = STYLE_TOC.unpack_from(data, 32 + i * 32)
        max_glyphs = max(max_glyphs, gc)
    return style_count, max_glyphs, path.stat().st_size
