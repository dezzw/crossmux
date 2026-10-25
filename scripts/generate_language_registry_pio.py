"""PlatformIO pre-build hook: refresh LanguageRegistry generated sources."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

Import("env")  # type: ignore[name-defined]  # noqa: F821

root = Path(env.subst("$PROJECT_DIR"))
script = root / "scripts/generate_language_registry.py"
cmd = [sys.executable, str(script)]
try:
    langs = env.GetProjectOption("hyphenation_langs")
except Exception:
    langs = None
if langs:
    cmd.extend(["--langs", langs])
subprocess.check_call(cmd, cwd=root)
