import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class ReleaseFontsWorkflowTest(unittest.TestCase):
    def test_font_release_never_marks_latest(self):
        workflow = (ROOT / ".github/workflows/release-fonts.yml").read_text()
        self.assertIn("permissions:", workflow)
        self.assertIn("contents: write", workflow)
        self.assertIn("${{ github.token }}", workflow)
        self.assertNotIn("FONTS_REPO_TOKEN", workflow)
        self.assertNotIn("crosspoint-fonts", workflow)
        self.assertIn('BASE="fonts-m', workflow)
        self.assertIn("concurrency:", workflow)
        self.assertIn("fonts-release", workflow)
        self.assertIn("verify_core_font_build.py", workflow)
        self.assertIn("fonts.immutable.json", workflow)
        self.assertNotIn("gh release delete", workflow)
        create_blocks = workflow.split("gh release create")
        self.assertGreaterEqual(len(create_blocks), 2)
        for block in create_blocks[1:]:
            segment = block.split("\n", 8)[0:8]
            joined = "\n".join(segment)
            self.assertIn("--latest=false", joined)

    def test_notosans_sc_pin_script_exists(self):
        self.assertTrue((ROOT / "scripts/fetch_notosans_sc_source.py").is_file())
        self.assertTrue((ROOT / "lib/EpdFont/scripts/font-release-pins.yaml").is_file())


if __name__ == "__main__":
    unittest.main()
