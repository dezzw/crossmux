#!/usr/bin/env python3
"""Check lossless font subsetting, shared indices and optional regeneration."""

import importlib.util
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = ROOT / "lib/EpdFont/scripts"
FONTS = ROOT / "lib/EpdFont/builtinFonts"

spec = importlib.util.spec_from_file_location("share_intervals", SCRIPTS / "share-cn-font-intervals.py")
share = importlib.util.module_from_spec(spec)
spec.loader.exec_module(share)


def array(text, suffix):
    return re.search(rf"\w+{suffix}\[\d*\] = \{{(.*?)\n\}};", text, re.S).group(1)


def rows(text, suffix):
    return [tuple(int(value.strip(), 0) for value in row.split(','))
            for row in re.findall(r"\{\s*([^{}]+?)\s*\}", array(text, suffix))]


class BuiltinFontShrinkTest(unittest.TestCase):
    def test_shared_outputs_are_current(self):
        for path, output in share.shared_outputs(FONTS):
            self.assertEqual(path.read_text(), output)

    def test_sharing_changes_only_interval_storage(self):
        shared = (FONTS / share.SHARED_FILE).read_text()
        body = array(shared, "Intervals")
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            for size in (8, 10, 12):
                name = f"notosans_cjk_{size}"
                text = (FONTS / f"{name}.h").read_text()
                text = text.replace(f'#include "{share.SHARED_FILE}"\n', '')
                text = text.replace(f'{share.SHARED_NAME},', f'{name}Intervals,')
                declaration = f'static const EpdUnicodeInterval {name}Intervals[] = {{{body}\n}};\n'
                # Restore the generator's original location, immediately before the font descriptor.
                text = text.replace(f'\n\nstatic const EpdFontData {name}',
                                    f'\n{declaration}\nstatic const EpdFontData {name}')
                (directory / f"{name}.h").write_text(text)
            for path, output in share.shared_outputs(directory):
                self.assertEqual(output, (FONTS / path.name).read_text())

    def test_mismatched_indices_do_not_write_anything(self):
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            for size in (8, 10, 12):
                offset = 1 if size == 12 else 0
                (directory / f"notosans_cjk_{size}.h").write_text(
                    f"static const EpdUnicodeInterval notosans_cjk_{size}Intervals[] = {{\n"
                    f"    {{ 0x20, 0x21, 0x{offset:X} }},\n}};\n")
            before = {path: path.read_bytes() for path in directory.iterdir()}
            result = subprocess.run([sys.executable, str(SCRIPTS / "share-cn-font-intervals.py"), str(directory)],
                                    capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("interval tables differ", result.stderr)
            self.assertEqual(before, {path: path.read_bytes() for path in directory.iterdir()})


if __name__ == "__main__":
    unittest.main()
