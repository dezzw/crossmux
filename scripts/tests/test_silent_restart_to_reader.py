"""Run the actual reader restart function with host platform stubs."""

import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]


class SilentRestartToReaderTest(unittest.TestCase):
    def test_reader_target_and_sleep_guard(self):
        source = (ROOT / "src/main.cpp").read_text()
        targets = source.split("RTC_NOINIT_ATTR uint32_t silentRebootMagic;", 1)[1].split(
            "// How the device is coming back to life", 1)[0]
        functions = "void silentRestart()" + source.split("void silentRestart()", 1)[1].split(
            "void silentRestartToReaderAndPreloadChineseFont", 1)[0]
        harness = r'''
#include <cassert>
#include <cstdint>
#define RTC_NOINIT_ATTR
#define LOG_DBG(...) ((void)0)
#define tr(key) "Loading"
uint32_t silentRebootMagic;
bool deepSleepInProgress = false;
unsigned restarts = 0, popups = 0, delayedMs = 0;
int renderer;
struct { void restart() { ++restarts; } } ESP;
struct { void drawPopup(int, const char*) { ++popups; } } GUI;
void delay(unsigned ms) { delayedMs += ms; }
'''
        checks = r'''
int main() {
  for (int si = 0; si < 2; ++si) {
    const bool sleeping = si != 0;
    deepSleepInProgress = sleeping;
    for (int pi = 0; pi < 2; ++pi) {
      const bool suppress = pi != 0;
      silentRebootMagic = 0;
      silentRebootTarget = static_cast<uint32_t>(SilentRebootTarget::Home);
      silentRebootFontPointSize = 24;
      restarts = popups = delayedMs = 0;
      silentRestartToReader(suppress);
      assert(restarts == !sleeping && popups == !sleeping);
      assert(delayedMs == (sleeping ? 0u : 50u));
      assert(silentRebootMagic == (sleeping ? 0u : SILENT_REBOOT_MAGIC));
      const auto expected = sleeping ? SilentRebootTarget::Home :
          suppress ? SilentRebootTarget::ReaderSuppressFontPrompt : SilentRebootTarget::Reader;
      assert(silentRebootTarget == static_cast<uint32_t>(expected));
      assert(silentRebootFontPointSize == (sleeping ? 24u : 0u));
    }
  }
}
'''
        with tempfile.TemporaryDirectory() as directory:
            cpp = Path(directory) / "restart.cpp"
            cpp.write_text(harness + targets + functions + checks)
            executable = Path(directory) / "restart"
            subprocess.run(shlex.split(os.environ.get("CXX", "g++")) + [
                "-std=c++20", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(executable)
            ], check=True, capture_output=True)
            subprocess.run([str(executable)], check=True, capture_output=True)
