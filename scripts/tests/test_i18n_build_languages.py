#!/usr/bin/env python3
"""Regression tests for [crosspoint] i18n_languages and gen_i18n --only-languages."""

import importlib.util
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def load_gen_i18n():
    spec = importlib.util.spec_from_file_location("gen_i18n", ROOT / "scripts" / "gen_i18n.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class I18nBuildLanguagesTest(unittest.TestCase):
    def test_platformio_lists_zh_cn(self):
        gen_i18n = load_gen_i18n()
        codes = gen_i18n.read_build_languages(str(ROOT))
        self.assertIsNotNone(codes)
        self.assertIn("ZH_CN", codes)

    def test_only_languages_emits_two_ui_languages(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "out"
            out.mkdir()
            subprocess.run(
                [
                    sys.executable,
                    str(ROOT / "scripts" / "gen_i18n.py"),
                    str(ROOT / "lib/I18n/translations"),
                    str(out),
                    "--only-languages",
                    "ZH_CN",
                    "--strip-unused",
                ],
                check=True,
                cwd=ROOT,
            )
            keys = (out / "I18nKeys.h").read_text(encoding="utf-8")
            self.assertIn("ZH_CN", keys)
            self.assertNotIn("Language::FR", keys)
            self.assertIn("getLanguageCount()", keys)
            self.assertIn("SORTED_LANGUAGE_INDICES[] = {0, 1}", keys)


if __name__ == "__main__":
    unittest.main()
