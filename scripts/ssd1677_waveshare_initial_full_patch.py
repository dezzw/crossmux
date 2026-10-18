"""Apply CrossMux SSD1677 Waveshare cold-boot FULL refresh overlay onto freeink-sdk."""

from __future__ import annotations

import subprocess
from pathlib import Path

PATCH_ROOT = Path(__file__).resolve().parents[1] / "crossmux-patches" / "ssd1677_waveshare_initial_full"
PATCH_FILE = PATCH_ROOT / "waveshare-initial-full.patch"

_DRIVER_CPP_REL = Path("libs/display/FreeInkDisplay/src/driver/CrossMuxSsd1677Driver.cpp")
_DRIVER_H_REL = Path("libs/display/FreeInkDisplay/src/driver/CrossMuxSsd1677Driver.h")


def verify_patch_applied(sdk_root: Path) -> None:
    """Ensure the Waveshare cold-boot overlay is present and patch did not leave rejects."""
    rej_files = list(sdk_root.rglob("*.rej"))
    if rej_files:
        joined = ", ".join(str(p.relative_to(sdk_root)) for p in rej_files[:5])
        extra = f" (+{len(rej_files) - 5} more)" if len(rej_files) > 5 else ""
        raise RuntimeError(f"SSD1677 patch left reject files under freeink-sdk: {joined}{extra}")

    driver_cpp = sdk_root / _DRIVER_CPP_REL
    driver_h = sdk_root / _DRIVER_H_REL
    if not driver_cpp.is_file() or not driver_h.is_file():
        raise RuntimeError(f"SSD1677 driver sources missing under {sdk_root}")

    cpp_text = driver_cpp.read_text(encoding="utf-8")
    h_text = driver_h.read_text(encoding="utf-8")
    if "initialFastPromoteFull" not in h_text:
        raise RuntimeError("SSD1677 Waveshare patch missing initialFastPromoteFull in CrossMuxSsd1677Driver.h")
    if "value.initialFastPromoteFull = true" not in cpp_text:
        raise RuntimeError(
            "SSD1677 Waveshare patch missing Waveshare config initialFastPromoteFull in CrossMuxSsd1677Driver.cpp"
        )
    if "_cfg.initialFastPromoteFull ? RefreshMode::Full" not in cpp_text:
        raise RuntimeError(
            "SSD1677 Waveshare patch missing resolveRefresh initialFastPromoteFull promotion in CrossMuxSsd1677Driver.cpp"
        )


def apply_patch(project_dir: Path) -> None:
    if not PATCH_FILE.is_file():
        raise FileNotFoundError(f"Missing SSD1677 patch: {PATCH_FILE}")
    sdk_root = project_dir / "freeink-sdk"
    if not sdk_root.is_dir():
        raise FileNotFoundError(f"Missing freeink-sdk submodule at {sdk_root}")

    try:
        verify_patch_applied(sdk_root)
        return
    except RuntimeError:
        pass

    result = subprocess.run(
        ["patch", "-p1", "--forward", "--batch"],
        cwd=sdk_root,
        input=PATCH_FILE.read_bytes(),
        capture_output=True,
    )
    stderr = result.stderr.decode(errors="replace")
    if result.returncode not in (0, 1):
        raise RuntimeError(f"SSD1677 patch failed (exit {result.returncode}): {stderr}")

    try:
        verify_patch_applied(sdk_root)
    except RuntimeError as exc:
        raise RuntimeError(
            f"SSD1677 Waveshare overlay verification failed after patch (exit {result.returncode}): {exc}"
        ) from exc


if __name__ == "__main__":
    apply_patch(Path(__file__).resolve().parents[1])
