"""Host regression tests for main-tab entry release barrier (ActivityManager)."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
ACTIVITY_MANAGER = ROOT / "src/activities/ActivityManager.cpp"


def method(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def host_cxx() -> str:
    for candidate in (os.environ.get("CXX"), "g++", "c++"):
        if candidate:
            return candidate
    return "g++"


def run_scenario(scenario: str, program: str) -> None:
    cxx = host_cxx()
    with tempfile.TemporaryDirectory(prefix="main-tab-entry-") as directory:
        cpp = Path(directory) / "check.cpp"
        exe = Path(directory) / "check"
        cpp.write_text(program)
        subprocess.run(
            [cxx, "-std=c++20", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(exe)],
            check=True,
        )
        subprocess.run([str(exe), scenario], check=True)


MAIN_TAB_H = (ROOT / "src/activities/MainTab.h").read_text().replace("#pragma once\n", "")

HOST_PREAMBLE = (
    r"""
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>

"""
    + MAIN_TAB_H
    + r"""

struct CrossPointSettings {
  bool standbyShortcutEnabled = false;
};
CrossPointSettings SETTINGS;

struct GfxRenderer {
  int getScreenWidth() const { return 800; }
};

struct UIThemeMetrics {
  int topPadding = 0;
  int headerHeight = 48;
};

struct UITheme {
  static UITheme& getInstance() {
    static UITheme theme;
    return theme;
  }
  bool hasMainTabs() const { return true; }
  const UIThemeMetrics& getMetrics() const { return metrics; }
  UIThemeMetrics metrics{};
};

class MappedInputManager {
 public:
  enum class Button : uint8_t {
    Back,
    Confirm,
    Left,
    Right,
    Up,
    Down,
  };

  uint16_t pressEdges = 0;
  uint16_t releaseEdges = 0;
  bool heldBack = false;

  static uint16_t bit(Button button) { return static_cast<uint16_t>(1u) << static_cast<uint8_t>(button); }

  bool wasPressed(Button button) const { return (pressEdges & bit(button)) != 0; }
  bool wasReleased(Button button) const { return (releaseEdges & bit(button)) != 0; }
  bool isPressed(Button button) const { return button == Button::Back && heldBack; }
  bool wasScreenTapped(int&, int&) const { return false; }
  bool wasScreenTouchDown(int&, int&) const { return false; }
};

struct Activity {
  virtual ~Activity() = default;
  virtual MainTab mainTab() const { return MainTab::Recent; }
  virtual void selectMainTabContentEdge(MainTabContentEdge) {}
  virtual bool mainTabBackReturnsToTabs() const { return true; }
  bool usesMainTabBar() const { return UITheme::getInstance().hasMainTabs() && mainTab() != MainTab::None; }
};

class ActivityManager {
 public:
  std::unique_ptr<Activity> currentActivity = std::make_unique<Activity>();
  MappedInputManager mappedInput{};
  GfxRenderer renderer{};
  MainTabFocus mainTabFocus = MainTabFocus::Tabs;
  bool mainTabEntryReleasePending = false;

  void goToMainTab(MainTab) {}
  void goToStandby() {}
  void requestUpdate() {}
"""
)

HOST_EPILOGUE = r"""
int main(int argc, char** argv) {
  if (argc < 2) return 2;
  const char* scenario = argv[1];
  if (strcmp(scenario, "same_frame_down") == 0) {
    ActivityManager manager;
    const auto down = MappedInputManager::bit(MappedInputManager::Button::Down);
    manager.mappedInput.pressEdges = down;
    manager.mappedInput.releaseEdges = down;
    assert(manager.handleMainTabInput());
    assert(!manager.mainTabEntryReleasePending);
    assert(manager.mainTabFocus == MainTabFocus::Content);
    return 0;
  }
  if (strcmp(scenario, "same_frame_up") == 0) {
    ActivityManager manager;
    const auto up = MappedInputManager::bit(MappedInputManager::Button::Up);
    manager.mappedInput.pressEdges = up;
    manager.mappedInput.releaseEdges = up;
    assert(manager.handleMainTabInput());
    assert(!manager.mainTabEntryReleasePending);
    assert(manager.mainTabFocus == MainTabFocus::Content);
    return 0;
  }
  if (strcmp(scenario, "down_press_only") == 0) {
    ActivityManager manager;
    manager.mappedInput.pressEdges = MappedInputManager::bit(MappedInputManager::Button::Down);
    assert(manager.handleMainTabInput());
    assert(manager.mainTabEntryReleasePending);
    assert(manager.mainTabFocus == MainTabFocus::Content);
    return 0;
  }
  if (strcmp(scenario, "up_press_only") == 0) {
    ActivityManager manager;
    manager.mappedInput.pressEdges = MappedInputManager::bit(MappedInputManager::Button::Up);
    assert(manager.handleMainTabInput());
    assert(manager.mainTabEntryReleasePending);
    assert(manager.mainTabFocus == MainTabFocus::Content);
    return 0;
  }
  if (strcmp(scenario, "barrier_swallows_until_release") == 0) {
    ActivityManager manager;
    manager.mappedInput.pressEdges = MappedInputManager::bit(MappedInputManager::Button::Down);
    assert(manager.handleMainTabInput());
    assert(manager.mainTabEntryReleasePending);
    manager.mappedInput.pressEdges = 0;
    manager.mappedInput.releaseEdges = 0;
    assert(manager.handleMainTabInput());
    assert(manager.mainTabEntryReleasePending);
    manager.mappedInput.releaseEdges = MappedInputManager::bit(MappedInputManager::Button::Down);
    assert(manager.handleMainTabInput());
    assert(!manager.mainTabEntryReleasePending);
    return 0;
  }
  return 2;
}
"""


def build_program() -> str:
    source = ACTIVITY_MANAGER.read_text()
    extracted = method(source, "bool ActivityManager::handleMainTabInput()")
    body_start = extracted.index("{") + 1
    body_end = extracted.rindex("}")
    body = extracted[body_start:body_end]
    return HOST_PREAMBLE + "  bool handleMainTabInput() {" + body + "  }\n};\n\n" + HOST_EPILOGUE


_PROGRAM: str | None = None


def program() -> str:
    global _PROGRAM
    if _PROGRAM is None:
        _PROGRAM = build_program()
    return _PROGRAM


class MainTabEntryReleaseBarrierTest(unittest.TestCase):
    def test_same_frame_down_does_not_set_entry_release_barrier(self) -> None:
        run_scenario("same_frame_down", program())

    def test_same_frame_up_does_not_set_entry_release_barrier(self) -> None:
        run_scenario("same_frame_up", program())

    def test_down_press_without_release_sets_entry_release_barrier(self) -> None:
        run_scenario("down_press_only", program())

    def test_up_press_without_release_sets_entry_release_barrier(self) -> None:
        run_scenario("up_press_only", program())

    def test_entry_release_barrier_swallows_frames_until_up_or_down_release(self) -> None:
        run_scenario("barrier_swallows_until_release", program())


if __name__ == "__main__":
    unittest.main()
