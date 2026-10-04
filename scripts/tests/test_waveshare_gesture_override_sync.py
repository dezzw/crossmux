import importlib.util
import sys
import unittest
from pathlib import Path


def load_sync_module():
    root = Path(__file__).resolve().parents[2]
    module_path = root / "scripts" / "waveshare397_gesture_override.py"
    spec = importlib.util.spec_from_file_location("waveshare397_gesture_override", module_path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module, root


class WaveshareGestureOverrideSyncTest(unittest.TestCase):
    def test_vendor_and_sdk_differ_before_sync(self):
        module, root = load_sync_module()
        vendor, sdk = module.gesture_paths(root)
        self.assertTrue(vendor.is_file(), vendor)
        self.assertTrue(sdk.is_file(), sdk)
        self.assertIn(b"bootDialUsed_", vendor.read_bytes())
        self.assertNotIn(b"bootDialUsed_", sdk.read_bytes())

    def test_sync_copies_vendor_header_into_sdk_include(self):
        module, root = load_sync_module()
        vendor, sdk = module.gesture_paths(root)
        original = sdk.read_bytes()
        try:
            changed = module.sync_gesture_override(root)
            self.assertTrue(changed)
            self.assertEqual(vendor.read_bytes(), sdk.read_bytes())
            self.assertIn(b"bootDialUsed_", sdk.read_bytes())

            changed_again = module.sync_gesture_override(root)
            self.assertFalse(changed_again)
        finally:
            sdk.write_bytes(original)


if __name__ == "__main__":
    unittest.main()
