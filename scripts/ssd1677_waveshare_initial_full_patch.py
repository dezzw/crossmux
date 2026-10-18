"""Apply CrossMux SSD1677 Waveshare cold-boot FULL refresh overlay onto freeink-sdk."""

from __future__ import annotations

import shutil
from pathlib import Path

PATCH_ROOT = Path(__file__).resolve().parents[1] / "crossmux-patches" / "ssd1677_waveshare_initial_full"


def apply_patch(project_dir: Path) -> None:
    source_root = PATCH_ROOT
    if not source_root.is_dir():
        raise FileNotFoundError(f"Missing SSD1677 patch tree: {source_root}")
    sdk_root = project_dir / "freeink-sdk"
    for path in source_root.rglob("*"):
        if not path.is_file():
            continue
        rel = path.relative_to(source_root)
        dest = sdk_root / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, dest)


if __name__ == "__main__":
    apply_patch(Path(__file__).resolve().parents[1])
