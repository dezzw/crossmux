"""Tests for scripts/generate_language_registry.py."""

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "scripts/generate_language_registry.py"


def _load_module():
    spec = importlib.util.spec_from_file_location("generate_language_registry", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


class GenerateLanguageRegistryTest(unittest.TestCase):
    def test_en_only_emits_single_include(self) -> None:
        mod = _load_module()
        with tempfile.TemporaryDirectory() as tmp:
            out_dir = Path(tmp)
            mod.GENERATED_DIR = out_dir
            mod.INCLUDES_PATH = out_dir / "LanguageRegistry.includes.generated.h"
            mod.BODY_PATH = out_dir / "LanguageRegistry.body.generated.inc"
            mod.CONFIG_PATH = out_dir / "LanguageRegistryConfig.generated.h"
            mod.write_outputs(["en"])

            includes = mod.INCLUDES_PATH.read_text()
            self.assertIn('#include "hyph-en.trie.h"', includes)
            self.assertNotIn("hyph-de.trie.h", includes)

            config = mod.CONFIG_PATH.read_text()
            self.assertIn("CROSSMUX_HYPHENATION_LANG_EN 1", config)
            self.assertNotIn("CROSSMUX_HYPHENATION_LANG_DE 1", config)

    def test_unknown_lang_rejected(self) -> None:
        mod = _load_module()
        with self.assertRaises(ValueError):
            mod._parse_lang_list("en,zz")


if __name__ == "__main__":
    unittest.main()
