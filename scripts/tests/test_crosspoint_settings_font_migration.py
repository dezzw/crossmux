"""Host-compile CrossPointSettings font-family migration extracted from production sources."""

from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def _extract_json_font_migration(settings_text: str) -> str:
    block = settings_text.split('  if (storedFontFamily == LEGACY_OPENDYSLEXIC', 1)[1]
    return (
        '  if (storedFontFamily == LEGACY_OPENDYSLEXIC'
        + block.split('  // Dictionary folder name', 1)[0]
    )


class CrossPointSettingsFontMigrationTest(unittest.TestCase):
    def test_legacy_opendyslexic_and_normal_font_family(self):
        settings = (ROOT / 'src/CrossPointSettings.cpp').read_text()
        json_migration = _extract_json_font_migration(settings)
        binary_migration = settings.split('  if (fileSettingsCount > 7) {', 1)[1]
        binary_migration = (
            '  if (fileSettingsCount > 7) {'
            + binary_migration.split('  if (fileSettingsCount > 8) {', 1)[0]
        )
        source = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>

struct CrossPointSettings {
  enum FONT_FAMILY { NOTOSERIF = 0, NOTOSANS = 1, FONT_FAMILY_COUNT = 2 };
  static constexpr uint8_t LEGACY_OPENDYSLEXIC = 2;
  static constexpr uint8_t BUILTIN_FONT_COUNT = FONT_FAMILY_COUNT;

  uint8_t fontFamily = NOTOSANS;
  uint8_t fontPointSize = 12;
  uint8_t sdFontFlashPreload = 0;
  char sdFontFamilyName[32] = {};
  bool lastNeedsResave = false;

  static uint8_t clamp(uint8_t val, uint8_t maxVal, uint8_t def) {
    return val < maxVal ? val : def;
  }

  void applyJsonFontFields(uint8_t storedFontFamily, const char* sdName) {
    bool needsResave = false;
    fontFamily = clamp(storedFontFamily, BUILTIN_FONT_COUNT, 0);
    strncpy(sdFontFamilyName, sdName, sizeof(sdFontFamilyName) - 1);
    sdFontFamilyName[sizeof(sdFontFamilyName) - 1] = '\0';
JSON_MIGRATION
    lastNeedsResave = needsResave;
  }

  void applyBinaryLegacyFontFamily(uint8_t legacyFontFamily) {
    std::array<uint8_t, 28> values{};
    values[7] = legacyFontFamily;
    const uint8_t fileSettingsCount = 8;
BINARY_MIGRATION
  }
};

int main() {
  CrossPointSettings settings;

  settings.applyJsonFontFields(CrossPointSettings::LEGACY_OPENDYSLEXIC, "");
  assert(settings.fontFamily == CrossPointSettings::NOTOSANS);
  assert(settings.sdFontFamilyName[0] == '\0');
  assert(settings.lastNeedsResave);

  settings = CrossPointSettings{};
  settings.applyJsonFontFields(CrossPointSettings::LEGACY_OPENDYSLEXIC, "Inter");
  assert(std::strcmp(settings.sdFontFamilyName, "Inter") == 0);
  assert(settings.fontFamily == CrossPointSettings::NOTOSERIF);
  assert(settings.lastNeedsResave);

  settings = CrossPointSettings{};
  settings.applyJsonFontFields(3, "");
  assert(settings.fontFamily == CrossPointSettings::NOTOSANS);
  assert(settings.lastNeedsResave);

  settings = CrossPointSettings{};
  settings.applyJsonFontFields(255, "");
  assert(settings.fontFamily == CrossPointSettings::NOTOSANS);
  assert(settings.lastNeedsResave);

  settings = CrossPointSettings{};
  settings.applyJsonFontFields(CrossPointSettings::NOTOSERIF, "");
  assert(settings.fontFamily == CrossPointSettings::NOTOSANS);
  assert(settings.lastNeedsResave);

  settings = CrossPointSettings{};
  settings.applyJsonFontFields(CrossPointSettings::NOTOSERIF, "NotoSerif");
  assert(settings.fontFamily == CrossPointSettings::NOTOSERIF);
  assert(std::strcmp(settings.sdFontFamilyName, "NotoSerif") == 0);
  assert(!settings.lastNeedsResave);

  settings = CrossPointSettings{};
  settings.applyBinaryLegacyFontFamily(CrossPointSettings::LEGACY_OPENDYSLEXIC);
  assert(settings.fontFamily == CrossPointSettings::NOTOSANS);
  assert(settings.sdFontFamilyName[0] == '\0');
  assert(settings.sdFontFlashPreload == 0);

  settings = CrossPointSettings{};
  strncpy(settings.sdFontFamilyName, "Inter", sizeof(settings.sdFontFamilyName) - 1);
  settings.sdFontFlashPreload = 1;
  settings.applyBinaryLegacyFontFamily(CrossPointSettings::LEGACY_OPENDYSLEXIC);
  assert(settings.fontFamily == CrossPointSettings::NOTOSANS);
  assert(settings.sdFontFamilyName[0] == '\0');
  assert(settings.sdFontFlashPreload == 0);

  settings = CrossPointSettings{};
  settings.applyBinaryLegacyFontFamily(CrossPointSettings::NOTOSERIF);
  assert(settings.fontFamily == CrossPointSettings::NOTOSERIF);
  return 0;
}
'''
        source = source.replace('JSON_MIGRATION', json_migration).replace('BINARY_MIGRATION', binary_migration)
        self.assertNotIn('OpenDyslexic', json_migration)
        self.assertNotIn('OpenDyslexic', binary_migration)
        with tempfile.TemporaryDirectory(prefix='cps-font-migration-') as directory:
            cpp = Path(directory) / 'migration.cpp'
            exe = Path(directory) / 'migration'
            cpp.write_text(source)
            subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    unittest.main()
