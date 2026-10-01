"""Ensure Waveshare builds compile the CrossMux FunctionButtonGesture override."""

from pathlib import Path

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


prepend_gesture_override(env)
