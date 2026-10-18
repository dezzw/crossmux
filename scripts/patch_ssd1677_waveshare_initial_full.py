"""PlatformIO pre-build: overlay Waveshare SSD1677 cold-boot FULL driver fix."""

from pathlib import Path
import sys

Import("env")  # noqa: F821

project_dir = Path(env.subst("$PROJECT_DIR"))
scripts_dir = project_dir / "scripts"
if str(scripts_dir) not in sys.path:
    sys.path.insert(0, str(scripts_dir))
from ssd1677_waveshare_initial_full_patch import apply_patch

apply_patch(project_dir)
