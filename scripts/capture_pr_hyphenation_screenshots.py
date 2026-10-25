#!/usr/bin/env python3
"""Capture simulator PNGs for hyphenation PR visual evidence."""

from __future__ import annotations

import argparse
import html
import json
import os
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / ".pio/build/simulator/program"

BOOKS = (
    ("en", "/english-longwords.epub", "English Long Words", "en", [
        "Extraordinary internationalization demonstrates hyphenation at narrow margins. "
        "Antidisestablishmentarianism and incomprehensibilities should break cleanly without "
        "distorting surrounding punctuation or spacing on the page.",
    ]),
    ("zh", "/chinese-sample.epub", "中文示例", "zh-CN", [
        "电子书阅读器应在中文段落中正常换行，不受西文连字符规则影响。"
        "这是一段用于视觉回归测试的简短正文。",
    ]),
    ("de", "/german-compound.epub", "Deutsch Verbindungen", "de", [
        "Quadratkilometer und Donaudampfschifffahrtsgesellschaftskapitän sind typische "
        "Komposita. Der Text soll sauber umbrechen, auch wenn keine Silbentrennung mehr "
        "eingefügt wird.",
    ]),
)


def _write_minimal_epub(path: Path, *, title: str, language: str, paragraphs: list[str]) -> None:
    body = "\n".join(f"<p>{html.escape(p)}</p>" for p in paragraphs)
    xhtml = f"""<?xml version="1.0" encoding="utf-8"?>
<html xmlns="http://www.w3.org/1999/xhtml" lang="{language}">
<head><title>{html.escape(title)}</title></head>
<body>{body}</body>
</html>"""
    opf = f"""<?xml version="1.0" encoding="utf-8"?>
<package version="3.0" unique-identifier="uid" xmlns="http://www.idpf.org/2007/opf">
  <metadata xmlns:dc="http://purl.org/dc/elements/1.1/">
    <dc:identifier id="uid">urn:uuid:pr-hyphenation-en</dc:identifier>
    <dc:title>{title}</dc:title>
    <dc:language>{language}</dc:language>
  </metadata>
  <manifest>
    <item id="c1" href="chapter.xhtml" media-type="application/xhtml+xml"/>
  </manifest>
  <spine><itemref idref="c1"/></spine>
</package>"""
    container = """<?xml version="1.0" encoding="UTF-8"?>
<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
  <rootfiles><rootfile full-path="OEBPS/content.opf" media-type="application/oebps-package+xml"/></rootfiles>
</container>"""
    path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(path, "w") as zf:
        zf.writestr("mimetype", "application/epub+zip", compress_type=zipfile.ZIP_STORED)
        zf.writestr("META-INF/container.xml", container)
        zf.writestr("OEBPS/content.opf", opf)
        zf.writestr("OEBPS/chapter.xhtml", xhtml)


def prepare_sd(root: Path, *, ui_language: str, recent_only: str | None = None) -> None:
    cp = root / ".crosspoint"
    cp.mkdir(parents=True, exist_ok=True)
    (cp / "user-guide.checked").write_text("1\n")
    settings = (
        '{"onboardingVersion":1,"language":"'
        + ui_language
        + '","contentProfile":1,"hyphenationEnabled":true,"fontFamily":2,"fontPointSize":16}'
    )
    (cp / "settings.json").write_text(settings)

    recent_books = []
    for key, sd_path, title, lang, paragraphs in BOOKS:
        _write_minimal_epub(root / sd_path.lstrip("/"), title=title, language=lang, paragraphs=paragraphs)
        if recent_only is None or recent_only == key:
            recent_books.append(
                {"path": sd_path, "title": title, "author": "CrossMux QA", "coverBmpPath": ""}
            )

    (cp / "recent.json").write_text(json.dumps({"books": recent_books}))


def chrome_script() -> str:
    return "2200:RIGHT;2500:RIGHT;2800:RIGHT;5200:LEFT;5500:LEFT;5800:LEFT;6500:QUIT"


def reader_script() -> str:
    return "5000:FUNCTION;6000:FUNCTION;12000:QUIT"


def reader_shot_ms() -> int:
    return 9000


def run_sim(sd: Path, *, script: str, shots: list[tuple[int, Path]]) -> None:
    if not SIM.is_file():
        raise SystemExit(f"Simulator binary missing: {SIM} (run pio run -e simulator first)")
    shot_spec = ";".join(f"{ms}:{path}" for ms, path in shots)
    env = {
        **os.environ,
        "CROSSPOINT_SIM_SD": str(sd),
        "CROSSPOINT_SIM_SCREENSHOTS": shot_spec,
        "CROSSPOINT_SIM_INPUT_SCRIPT": script,
    }
    subprocess.run([str(SIM)], env=env, cwd=ROOT, check=True, timeout=120)


def bmp_to_png(bmp: Path) -> None:
    from PIL import Image

    png = bmp.with_suffix(".png")
    Image.open(bmp).save(png)
    bmp.unlink()


def capture_label(out_dir: Path, *, ui_language: str, label: str) -> None:
    sd = out_dir / f"sd-{label}"
    if sd.exists():
        shutil.rmtree(sd)
    prepare_sd(sd, ui_language=ui_language)
    bmp_dir = out_dir / label
    bmp_dir.mkdir(parents=True, exist_ok=True)

    run_sim(
        sd,
        script=chrome_script(),
        shots=[
            (1200, bmp_dir / f"{label}-home.bmp"),
            (3600, bmp_dir / f"{label}-settings.bmp"),
        ],
    )

    for key, *_rest in BOOKS:
        reader_bmp = bmp_dir / f"{label}-reader-{key}.bmp"
        reader_sd = out_dir / f"sd-{label}-reader-{key}"
        if reader_sd.exists():
            shutil.rmtree(reader_sd)
        prepare_sd(reader_sd, ui_language=ui_language, recent_only=key)
        run_sim(reader_sd, script=reader_script(), shots=[(reader_shot_ms(), reader_bmp)])

    for bmp in bmp_dir.glob("*.bmp"):
        bmp_to_png(bmp)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, default=ROOT / "docs/screenshots/pr-hyphenation-en")
    parser.add_argument("--label", required=True)
    parser.add_argument("--ui-language", default="EN")
    args = parser.parse_args()
    capture_label(args.out, ui_language=args.ui_language, label=args.label)
    return 0


if __name__ == "__main__":
    sys.exit(main())
