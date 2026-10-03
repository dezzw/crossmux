"""Ensure Waveshare builds compile the CrossMux FunctionButtonGesture override."""

from pathlib import Path
import sys

Import("env")  # noqa: F821

OVERRIDE_INCLUDE = "$PROJECT_DIR/lib/waveshare397_input/include"


def prepend_gesture_override(build_env):
    include = build_env.subst(OVERRIDE_INCLUDE)
    if not Path(include).is_dir():
        raise RuntimeError(f"Missing Waveshare gesture override include dir: {include}")
    flags = build_env.get("CPPFLAGS", [])
    prefix = ["-I" + include]
    if prefix[0] not in flags:
        build_env["CPPFLAGS"] = prefix + list(flags)


def install_gesture_override(build_env):
    project_dir = Path(build_env.subst("$PROJECT_DIR"))
    scripts_dir = project_dir / "scripts"
    if str(scripts_dir) not in sys.path:
        sys.path.insert(0, str(scripts_dir))
    from waveshare397_gesture_override import sync_gesture_override

    sync_gesture_override(project_dir)


prepend_gesture_override(env)
install_gesture_override(env)
