#!/usr/bin/env python3
"""Regression: WiFi network list must converge ListNav follow() like UiListActivity."""

from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
WIFI_CPP = REPO / "src/activities/network/WifiSelectionActivity.cpp"


def test_wifi_list_uses_list_nav_rebuild_loop() -> None:
    text = WIFI_CPP.read_text(encoding="utf-8")
    assert "consumeRebuildNeeded()" in text, "WiFi list render must replay while ListNav follow() converges"
    assert "onNextRelease" in text, "WiFi list must use release-based list navigation contract"
    assert "onPreviousRelease" in text
    assert "onNextContinuous" in text
    assert "onPreviousContinuous" in text


if __name__ == "__main__":
    test_wifi_list_uses_list_nav_rebuild_loop()
    print("ok")
