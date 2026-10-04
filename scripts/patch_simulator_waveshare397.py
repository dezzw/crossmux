"""Install Waveshare 3.97 simulator HalGPIO overlay into the pinned crosspoint-simulator lib."""

import shutil
from pathlib import Path

Import("env")  # noqa: F821


def simulator_halgpio_paths(build_env):
    project = Path(build_env.subst("$PROJECT_DIR"))
    libdeps = project / ".pio" / "libdeps" / build_env.subst("$PIOENV")
    if not libdeps.is_dir():
        return None, None, None, None
    for src_dir in libdeps.glob("*/src"):
        hal_cpp = src_dir / "HalGPIO.cpp"
        hal_h = src_dir / "HalGPIO.h"
        if hal_cpp.is_file() and hal_h.is_file():
            overlay_dir = project / "lib" / "simulator_waveshare397_overlay"
            return hal_cpp, hal_h, overlay_dir / "HalGPIO.cpp", overlay_dir / "HalGPIO.h"
    return None, None, None, None


def install_overlay(build_env):
    if build_env.subst("$PIOENV") != "simulator":
        return
    dst_cpp, dst_h, src_cpp, src_h = simulator_halgpio_paths(build_env)
    if dst_cpp is None:
        raise RuntimeError(
            "Waveshare simulator overlay: crosspoint-simulator HalGPIO not found under .pio/libdeps/simulator "
            "(run pio pkg install -e simulator first)"
        )
    if not src_cpp.is_file() or not src_h.is_file():
        raise RuntimeError(f"Missing overlay sources: {src_cpp} / {src_h}")
    shutil.copy2(src_cpp, dst_cpp)
    shutil.copy2(src_h, dst_h)


# Copy before the simulator library compiles HalGPIO.cpp (libdeps are installed
# before project pre-scripts run in a normal `pio run -e simulator`).
install_overlay(env)

overlay_cpp = Path(env.subst("$PROJECT_DIR/lib/simulator_waveshare397_overlay/HalGPIO.cpp"))
if overlay_cpp.is_file():
    env.Depends("$BUILD_DIR/${PROGNAME}.elf", overlay_cpp)
