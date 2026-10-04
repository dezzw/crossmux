#!/usr/bin/env python3
"""Regression: WiFi network list must converge ListNav follow() like UiListActivity."""

from pathlib import Path
import re
import unittest

REPO = Path(__file__).resolve().parents[2]
WIFI_CPP = REPO / "src/activities/network/WifiSelectionActivity.cpp"


def extract_function(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def extract_if_block(source: str, condition: str) -> str:
    needle = f"if ({condition})"
    start = source.index(needle)
    brace = source.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


class WifiListNavRebuildTest(unittest.TestCase):
    def test_render_network_list_rebuild_loop(self) -> None:
        text = WIFI_CPP.read_text(encoding="utf-8")
        render_fn = extract_function(text, "void WifiSelectionActivity::renderNetworkList")
        self.assertIn("renderUi();", render_fn, "renderNetworkList must draw before checking rebuild")
        self.assertRegex(
            render_fn,
            r"for\s*\(\s*int\s+pass\s*=\s*0\s*;[^;]*consumeRebuildNeeded\(\)[^;]*pass\s*<\s*8",
            "rebuild loop must live inside renderNetworkList and cap passes at 8",
        )
        self.assertNotRegex(
            render_fn,
            r"buttonNavigator\.onNext(?:Release|Continuous)?\s*\(",
            "renderNetworkList must not register button list navigation",
        )

    def test_network_list_uses_release_not_press_navigation(self) -> None:
        text = WIFI_CPP.read_text(encoding="utf-8")
        network_list = extract_if_block(text, "state == WifiSelectionState::NETWORK_LIST")
        self.assertIn("buttonNavigator.onNextRelease", network_list)
        self.assertIn("buttonNavigator.onPreviousRelease", network_list)
        self.assertIn("buttonNavigator.onNextContinuous", network_list)
        self.assertIn("buttonNavigator.onPreviousContinuous", network_list)
        self.assertNotIn("buttonNavigator.onNext(", network_list)
        self.assertNotIn("buttonNavigator.onPrevious(", network_list)


if __name__ == "__main__":
    unittest.main()
