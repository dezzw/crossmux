"""Ensure Waveshare builds compile the CrossMux FunctionButtonGesture override."""

from pathlib import Path
import sys

Import("env")  # noqa: F821

project_dir = Path(env.subst("$PROJECT_DIR"))
scripts_dir = project_dir / "scripts"
if str(scripts_dir) not in sys.path:
    sys.path.insert(0, str(scripts_dir))
from waveshare397_gesture_override import sync_gesture_override

sync_gesture_override(project_dir)
