"""Apply CrossMux SSD1677 Waveshare cold-boot FULL refresh overlay onto freeink-sdk."""

from __future__ import annotations

import subprocess
from pathlib import Path

PATCH_ROOT = Path(__file__).resolve().parents[1] / "crossmux-patches" / "ssd1677_waveshare_initial_full"
PATCH_FILE = PATCH_ROOT / "waveshare-initial-full.patch"


def apply_patch(project_dir: Path) -> None:
    if not PATCH_FILE.is_file():
        raise FileNotFoundError(f"Missing SSD1677 patch: {PATCH_FILE}")
    sdk_root = project_dir / "freeink-sdk"
    result = subprocess.run(
        ["patch", "-p1", "--forward", "--batch"],
        cwd=sdk_root,
        input=PATCH_FILE.read_bytes(),
        capture_output=True,
    )
    if result.returncode not in (0, 1):  # 1 = already applied
        raise RuntimeError(
            f"SSD1677 patch failed (exit {result.returncode}): {result.stderr.decode(errors='replace')}"
        )


if __name__ == "__main__":
    apply_patch(Path(__file__).resolve().parents[1])
