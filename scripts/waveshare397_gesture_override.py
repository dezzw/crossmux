"""Waveshare 3.97 FunctionButtonGesture vendor override install helpers."""

from __future__ import annotations

from pathlib import Path

VENDOR_REL = Path("lib/waveshare397_input/include/FunctionButtonGesture.h")
SDK_REL = Path("freeink-sdk/libs/hardware/InputManager/include/FunctionButtonGesture.h")


def gesture_paths(project_dir: Path) -> tuple[Path, Path]:
    root = project_dir.resolve()
    return root / VENDOR_REL, root / SDK_REL


def sync_gesture_override(project_dir: Path, *, dry_run: bool = False) -> bool:
    """Install the CrossMux vendor header where InputManager.h includes it.

    InputManager.h uses #include \"FunctionButtonGesture.h\" from its own include/
    directory. GCC resolves that quoted include before any -I path, so prepending
    lib/waveshare397_input/include to CPPFLAGS never replaced the SDK copy. Copy
    the vendor header over the SDK twin before compiling InputManager.
    """
    vendor, sdk = gesture_paths(project_dir)
    if not vendor.is_file():
        raise FileNotFoundError(f"Missing vendor gesture header: {vendor}")
    if not sdk.parent.is_dir():
        raise FileNotFoundError(
            f"Missing InputManager include dir (run git submodule update --init freeink-sdk): {sdk.parent}"
        )

    vendor_bytes = vendor.read_bytes()
    if sdk.is_symlink():
        sdk.unlink()
    if sdk.is_file() and sdk.read_bytes() == vendor_bytes:
        return False
    if dry_run:
        return True
    sdk.write_bytes(vendor_bytes)
    return True
