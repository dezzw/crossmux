#!/usr/bin/env python3
"""Load v4 .cpfont styles and render text like GfxRenderer (baseline, fp4, 2-bit)."""

from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path

STYLE_TOC = struct.Struct("<B3xIIBhhHHBBBI4x")
GLYPH_FMT = struct.Struct("<BBHhhH2xI")
INTERVAL_FMT = struct.Struct("<III")
KERN_ENTRY_FMT = struct.Struct("<HB")
FP4_HALF = 8


def fp4_to_pixel(value: int) -> int:
    return (int(value) + FP4_HALF) >> 4


@dataclass
class CpGlyph:
    width: int
    height: int
    advance_x: int
    left: int
    top: int
    data_length: int
    data_offset: int
    bitmap: bytes


@dataclass
class CpFontStyle:
    style_id: int
    ascender: int
    descender: int
    advance_y: int
    is_2bit: bool
    intervals: list[tuple[int, int, int]]
    glyphs: list[CpGlyph]
    kern_left: dict[int, int]
    kern_right: dict[int, int]
    kern_matrix: bytes
    kern_left_class_count: int
    kern_right_class_count: int

    def find_glyph_index(self, codepoint: int) -> int | None:
        for first, last, offset in self.intervals:
            if first <= codepoint <= last:
                return offset + (codepoint - first)
        return None

    def get_glyph(self, codepoint: int) -> CpGlyph | None:
        idx = self.find_glyph_index(codepoint)
        if idx is None or idx >= len(self.glyphs):
            return None
        g = self.glyphs[idx]
        if g.data_length == 0 or g.width == 0:
            return None
        return g

    def kern_class_left(self, cp: int) -> int:
        return self.kern_left.get(cp, 0)

    def kern_class_right(self, cp: int) -> int:
        return self.kern_right.get(cp, 0)

    def get_kerning(self, left_cp: int, right_cp: int) -> int:
        lc = self.kern_class_left(left_cp)
        rc = self.kern_class_right(right_cp)
        if lc == 0 or rc == 0 or self.kern_left_class_count == 0 or self.kern_right_class_count == 0:
            return 0
        idx = (lc - 1) * self.kern_right_class_count + (rc - 1)
        if idx < 0 or idx >= len(self.kern_matrix):
            return 0
        return self.kern_matrix[idx]

    def measure_width(self, text: str) -> int:
        width = 0
        prev_cp = 0
        prev_advance = 0
        have_prev = False
        for ch in text:
            if ch == "\n":
                break
            cp = ord(ch)
            glyph = self.get_glyph(cp)
            if have_prev and prev_cp:
                width += fp4_to_pixel(prev_advance + self.get_kerning(prev_cp, cp))
            elif have_prev:
                width += fp4_to_pixel(prev_advance)
            prev_advance = glyph.advance_x if glyph else fp4_to_pixel(self.ascender) << 4
            prev_cp = cp
            have_prev = True
        if have_prev:
            width += fp4_to_pixel(prev_advance)
        return width


def _coverage_2bit(bitmap: bytes, pixel_index: int) -> int:
    byte = bitmap[pixel_index >> 2]
    shift = (3 - (pixel_index & 3)) * 2
    return (byte >> shift) & 0x3


def draw_glyph_1bit(pixels: bytearray, width: int, height: int, glyph: CpGlyph, cursor_x: int, baseline_y: int) -> None:
    if not glyph.bitmap:
        return
    base_x = cursor_x + glyph.left
    base_y = baseline_y - glyph.top
    for gy in range(glyph.height):
        for gx in range(glyph.width):
            pos = gy * glyph.width + gx
            if _coverage_2bit(glyph.bitmap, pos) >= 2:
                x = base_x + gx
                y = base_y + gy
                if 0 <= x < width and 0 <= y < height:
                    pixels[y * width + x] = 0


def draw_text_line(
    pixels: bytearray,
    screen_w: int,
    screen_h: int,
    style: CpFontStyle,
    text: str,
    x: int,
    baseline_y: int,
) -> None:
    cursor_x = x
    prev_cp = 0
    prev_advance = 0
    have_prev = False
    for ch in text:
        cp = ord(ch)
        glyph = style.get_glyph(cp)
        if have_prev and prev_cp:
            cursor_x += fp4_to_pixel(prev_advance + style.get_kerning(prev_cp, cp))
        elif have_prev:
            cursor_x += fp4_to_pixel(prev_advance)
        if glyph:
            draw_glyph_1bit(pixels, screen_w, screen_h, glyph, cursor_x, baseline_y)
            prev_advance = glyph.advance_x
        else:
            prev_advance = fp4_to_pixel(style.ascender) << 4
        prev_cp = cp
        have_prev = True


def wrap_text(style: CpFontStyle, text: str, max_width: int) -> list[str]:
    words = text.split()
    if not words:
        return [""]
    lines: list[str] = []
    current = words[0]
    for word in words[1:]:
        trial = f"{current} {word}"
        if style.measure_width(trial) <= max_width:
            current = trial
        else:
            lines.append(current)
            current = word
    lines.append(current)
    return lines


def load_cpfont_style(path: Path, style_id: int = 0) -> CpFontStyle:
    data = path.read_bytes()
    if data[:6] != b"CPFONT":
        raise ValueError(f"not CPFONT: {path}")
    flags = struct.unpack_from("<H", data, 10)[0]
    is_2bit = bool(flags & 1)
    style_count = data[12]
    toc_base = 32
    entry = None
    for i in range(style_count):
        chunk = STYLE_TOC.unpack_from(data, toc_base + i * 32)
        if chunk[0] == style_id:
            entry = chunk
            break
    if entry is None:
        raise ValueError(f"style {style_id} not in {path}")

    (
        sid,
        interval_count,
        glyph_count,
        advance_y,
        ascender,
        descender,
        kern_left_count,
        kern_right_count,
        kern_left_class_count,
        kern_right_class_count,
        ligature_count,
        data_offset,
    ) = entry

    pos = data_offset
    intervals: list[tuple[int, int, int]] = []
    for _ in range(interval_count):
        intervals.append(INTERVAL_FMT.unpack_from(data, pos))
        pos += 12

    glyphs_raw: list[tuple] = []
    for _ in range(glyph_count):
        glyphs_raw.append(GLYPH_FMT.unpack_from(data, pos))
        pos += 16

    kern_left: dict[int, int] = {}
    for _ in range(kern_left_count):
        cp, cls = KERN_ENTRY_FMT.unpack_from(data, pos)
        kern_left[cp] = cls
        pos += 3

    kern_right: dict[int, int] = {}
    for _ in range(kern_right_count):
        cp, cls = KERN_ENTRY_FMT.unpack_from(data, pos)
        kern_right[cp] = cls
        pos += 3

    matrix_len = max(0, kern_left_class_count * kern_right_class_count)
    if matrix_len:
        kern_matrix = struct.unpack(f"<{matrix_len}b", data[pos : pos + matrix_len])
    else:
        kern_matrix = ()
    pos += matrix_len
    pos += ligature_count * 8

    bitmap_blob = data[pos:]
    glyphs: list[CpGlyph] = []
    for w, h, ax, left, top, dl, doff in glyphs_raw:
        bmp = bitmap_blob[doff : doff + dl] if dl else b""
        glyphs.append(CpGlyph(w, h, ax, left, top, dl, doff, bmp))

    return CpFontStyle(
        style_id=sid,
        ascender=ascender,
        descender=descender,
        advance_y=advance_y,
        is_2bit=is_2bit,
        intervals=intervals,
        glyphs=glyphs,
        kern_left=kern_left,
        kern_right=kern_right,
        kern_matrix=kern_matrix,
        kern_left_class_count=kern_left_class_count,
        kern_right_class_count=kern_right_class_count,
    )
