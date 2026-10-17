#!/usr/bin/env python3
"""Extract codepoint coverage from v4 .cpfont files."""

from __future__ import annotations

import struct
from pathlib import Path

STYLE_TOC = struct.Struct("<B3xIIBhhHHBBBI4x")
GLYPH = struct.Struct("<BBHhhH2xI")


def _style_toc_entry(data: bytes, style_id: int) -> tuple:
    style_count = data[12]
    for i in range(style_count):
        chunk = STYLE_TOC.unpack_from(data, 32 + i * 32)
        if chunk[0] == style_id:
            return chunk
    raise ValueError(f"style {style_id} not in cpfont")


def cpfont_codepoints(path: Path, *, style_id: int = 0, require_bitmap: bool = False) -> list[int]:
    data = path.read_bytes()
    if data[:6] != b"CPFONT":
        raise ValueError(f"not a CPFONT file: {path}")
    _sid, interval_count, glyph_count, *_rest, data_offset = _style_toc_entry(data, style_id)

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
