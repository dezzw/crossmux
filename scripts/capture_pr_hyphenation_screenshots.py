#!/usr/bin/env python3
"""Capture simulator BMPs for hyphenation PR visual evidence."""

from __future__ import annotations

import argparse
import html
import os
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / ".pio/build/simulator/program"


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


def prepare_sd(root: Path, *, ui_language: str) -> None:
    cp = root / ".crosspoint"
    cp.mkdir(parents=True, exist_ok=True)
    (cp / "user-guide.checked").write_text("1\n")
    settings = (
        '{"onboardingVersion":1,"language":"'
        + ui_language
        + '","contentProfile":1,"hyphenationEnabled":true,"fontFamily":2,"fontPointSize":16}'
    )
    (cp / "settings.json").write_text(settings)

    en_path = "/english-longwords.epub"
    de_path = "/german-compound.epub"
    zh_path = "/chinese-sample.epub"

    _write_minimal_epub(
        root / en_path.lstrip("/"),
        title="English Long Words",
        language="en",
        paragraphs=[
            "Extraordinary internationalization demonstrates hyphenation at narrow margins. "
            "Antidisestablishmentarianism and incomprehensibilities should break cleanly without "
            "distorting surrounding punctuation or spacing on the page.",
        ],
    )
    _write_minimal_epub(
        root / de_path.lstrip("/"),
        title="Deutsch Verbindungen",
        language="de",
        paragraphs=[
            "Quadratkilometer und Donaudampfschifffahrtsgesellschaftskapitän sind typische "
            "Komposita. Der Text soll sauber umbrechen, auch wenn keine Silbentrennung mehr "
            "eingefügt wird.",
        ],
    )
    _write_minimal_epub(
        root / zh_path.lstrip("/"),
        title="中文示例",
        language="zh-CN",
        paragraphs=[
            "电子书阅读器应在中文段落中正常换行，不受西文连字符规则影响。"
            "这是一段用于视觉回归测试的简短正文。",
        ],
    )

    recent = {
        "books": [
            {
                "path": en_path,
                "title": "English Long Words",
                "author": "CrossMux QA",
                "coverBmpPath": "",
            },
            {
                "path": de_path,
                "title": "Deutsch Verbindungen",
                "author": "CrossMux QA",
                "coverBmpPath": "",
            },
            {
                "path": zh_path,
                "title": "中文示例",
                "author": "CrossMux QA",
                "coverBmpPath": "",
            },
        ]
    }
    import json

    (cp / "recent.json").write_text(json.dumps(recent))


def run_capture(out_dir: Path, *, ui_language: str, label: str) -> None:
    if not SIM.is_file():
        raise SystemExit(f"Simulator binary missing: {SIM} (run pio run -e simulator first)")

    sd = out_dir / f"sd-{label}"
    if sd.exists():
        shutil.rmtree(sd)
    prepare_sd(sd, ui_language=ui_language)
    bmp_dir = out_dir / label
    bmp_dir.mkdir(parents=True, exist_ok=True)

    # Times are ms from simulator start. Waveshare profile: RIGHT/LEFT tabs, ENTER confirms.
    shots = [
        (4500, bmp_dir / f"{label}-home.bmp"),
        (7000, bmp_dir / f"{label}-settings.bmp"),
        (12000, bmp_dir / f"{label}-reader-en.bmp"),
        (17000, bmp_dir / f"{label}-reader-zh.bmp"),
        (22000, bmp_dir / f"{label}-reader-de.bmp"),
    ]
    shot_spec = ";".join(f"{ms}:{path}" for ms, path in shots)
    script = (
        "2000:RIGHT;2300:RIGHT;2600:RIGHT;"  # Recent -> Settings tab
        "3200:DOWN;3600:ENTER;"  # open settings list
        "8500:BACK;8800:BACK;"  # back to Recent tab
        "9500:ENTER;"  # open first recent (English)
        "10000:DOWN;10500:DOWN;"  # page turns
        "14000:BACK;"
        "14500:DOWN;15000:ENTER;"  # second recent (Chinese)
        "19000:BACK;"
        "19500:DOWN;20000:DOWN;20500:ENTER;"  # third recent (German)
        "21500:DOWN;23000:QUIT"
    )
    env = {
        **os.environ,
        "CROSSPOINT_SIM_SD": str(sd),
        "CROSSPOINT_SIM_SCREENSHOTS": shot_spec,
        "CROSSPOINT_SIM_INPUT_SCRIPT": script,
    }
    subprocess.run([str(SIM)], env=env, cwd=ROOT, check=True, timeout=120)


def bmp_to_png(bmp: Path) -> Path:
    from PIL import Image

    png = bmp.with_suffix(".png")
    Image.open(bmp).save(png)
    return png


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, default=ROOT / "docs/screenshots/pr-hyphenation-en")
    parser.add_argument("--label", required=True, help="Subfolder label, e.g. after-main or after-en-only")
    parser.add_argument("--ui-language", default="EN", help="Settings language code (EN or ZH_CN)")
    args = parser.parse_args()

    run_capture(args.out, ui_language=args.ui_language, label=args.label)
    for bmp in sorted((args.out / args.label).glob("*.bmp")):
        bmp_to_png(bmp)
    return 0


if __name__ == "__main__":
    sys.exit(main())
