"""Host-compile CrossPointSettings language JSON block extracted from production sources."""

from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
START = "  // Language -- stored as code string for stability across enum reorders."
END = "  // Migrate the split-build region marker"


def _extract_language_block(settings_text: str) -> str:
    if START not in settings_text:
        raise ValueError(f"missing language block start marker: {START!r}")
    tail = settings_text.split(START, 1)[1]
    if END not in tail:
        raise ValueError(f"missing language block end marker: {END!r}")
    return START + tail.split(END, 1)[0]


HOST = r"""
#include <I18n.h>
#include <util/StoredLanguageCode.h>

#include <cassert>
#include <cstdint>
#include <cstring>

struct LangField {
  const char* code;
  template <typename T> bool is() const { return code != nullptr; }
  template <typename T> const char* as() const { return code; }
};

struct JsonDoc {
  const char* languageCode;
  LangField operator[](const char* key) const {
    return LangField{std::strcmp(key, "language") == 0 ? languageCode : nullptr};
  }
};

struct CrossPointSettings {
  uint8_t language = 0;
  bool lastNeedsResave = false;

  void applyLanguageFromJson(const char* storedCode) {
    JsonDoc doc{storedCode};
    bool needsResave = false;
LANGUAGE_BLOCK
    lastNeedsResave = needsResave;
  }
};

static void expect(const char* code, uint8_t lang, bool resave) {
  CrossPointSettings s{};
  s.applyLanguageFromJson(code);
  assert(s.language == lang);
  assert(s.lastNeedsResave == resave);
}

int main() {
  expect("FR", static_cast<uint8_t>(Language::EN), true);
  expect("EN", static_cast<uint8_t>(Language::EN), false);
  expect("ZH_CN", static_cast<uint8_t>(Language::ZH_CN), false);
  expect("en", static_cast<uint8_t>(Language::EN), true);
  expect("XX", static_cast<uint8_t>(Language::EN), true);
  return 0;
}
"""


class CrossPointSettingsLanguageResaveTest(unittest.TestCase):
    def test_extracted_language_block(self):
        block = _extract_language_block((ROOT / "src/CrossPointSettings.cpp").read_text())
        source = HOST.replace("LANGUAGE_BLOCK", block)
        with tempfile.TemporaryDirectory(prefix="cps-lang-resave-") as directory:
            cpp = Path(directory) / "language_resave.cpp"
            exe = Path(directory) / "language_resave"
            cpp.write_text(source)
            subprocess.run(
                [
                    "g++",
                    "-std=c++20",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    "-DENABLE_CHINESE_VERSION=1",
                    f"-I{ROOT / 'src'}",
                    f"-I{ROOT / 'lib' / 'I18n'}",
                    str(cpp),
                    str(ROOT / "lib/I18n/I18n.cpp"),
                    str(ROOT / "lib/I18n/I18nStrings.cpp"),
                    "-o",
                    str(exe),
                ],
                check=True,
            )
            subprocess.run([str(exe)], check=True)

    def test_missing_extraction_marker_fails(self):
        with self.assertRaises(ValueError):
            _extract_language_block("")


if __name__ == "__main__":
    unittest.main()
