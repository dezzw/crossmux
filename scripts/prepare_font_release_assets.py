#!/usr/bin/env python3
"""Copy OFL texts and write NOTICE files for font GitHub release assets."""

from __future__ import annotations

import argparse
import urllib.request
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
PINS = ROOT / "lib/EpdFont/scripts/font-release-pins.yaml"


def download(url: str, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    urllib.request.urlretrieve(url, dest)


def write_notice(dest: Path, family: str, meta: dict) -> None:
    repo = meta["repository"]
    commit = meta["commit"]
    lines = [
        f"{family} — upstream source pin",
        f"Repository: https://github.com/{repo}",
        f"Commit: {commit}",
        "",
        "Font files were built from the URLs declared in lib/EpdFont/scripts/sd-fonts.yaml",
        "for this family at the commit above.",
    ]
    if family == "NotoSansSC":
        lines.extend(
            [
                "",
                "© 2014-2021 Adobe (http://www.adobe.com/), with Reserved Font Name Source",
                "",
                f"Hanzi allowlist: lib/EpdFont/scripts/{meta.get('hanzi_allowlist', 'noto_m1_b4_hanzi_coverage.txt')}",
            ]
        )
        for src in meta.get("sources", []):
            lines.extend(
                [
                    "",
                    f"Master: {src['relative_path']}",
                    f"Download URL: {src['url']}",
                    f"SHA-256: {src['sha256']}",
                ]
            )
    dest.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True, help="Directory for OFL/NOTICE assets")
    args = parser.parse_args()
    pins = yaml.safe_load(PINS.read_text())
    for family, meta in pins.items():
        ofl_path = args.output / f"{family}-OFL.txt"
        notice_path = args.output / f"{family}-NOTICE.txt"
        download(meta["ofl_url"], ofl_path)
        write_notice(notice_path, family, meta)
        print(f"Prepared {ofl_path.name} and {notice_path.name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
