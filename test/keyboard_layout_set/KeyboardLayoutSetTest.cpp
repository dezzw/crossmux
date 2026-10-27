#include <KeyboardLayoutSet.h>
#include <gtest/gtest.h>

#include "CrossPointSettings.h"
#include "I18n.h"
#include "I18nKeys.h"

namespace {

constexpr uint16_t kMaskEnFrRuHe =
    keyboard_layouts::bitAt(0) | keyboard_layouts::bitAt(1) | keyboard_layouts::bitAt(4) | keyboard_layouts::bitAt(8);
constexpr uint16_t kMaskAllNine = static_cast<uint16_t>((1u << keyboard_layouts::COUNT) - 1u);

constexpr const char* kDisplayNameEn = "English";
constexpr const char* kDisplayNameFr = "Français";
constexpr const char* kDisplayNameDe = "Deutsch";
constexpr const char* kDisplayNameEs = "Español";
constexpr const char* kDisplayNameRu = "Русский";
constexpr const char* kDisplayNameUk = "Українська";
constexpr const char* kDisplayNameBe = "Беларуская";
constexpr const char* kDisplayNameKk = "Қазақша";
constexpr const char* kDisplayNameHe = "עברית";

constexpr const char* kExpectedDisplayNames[] = {kDisplayNameEn, kDisplayNameFr, kDisplayNameDe,
                                                 kDisplayNameEs, kDisplayNameRu, kDisplayNameUk,
                                                 kDisplayNameBe, kDisplayNameKk, kDisplayNameHe};

constexpr freeink::ui::KeyboardLayoutId kExpectedIds[] = {
    freeink::ui::KeyboardLayoutId::QwertyEn,   freeink::ui::KeyboardLayoutId::AzertyFr,
    freeink::ui::KeyboardLayoutId::QwertzDe,   freeink::ui::KeyboardLayoutId::SpanishEs,
    freeink::ui::KeyboardLayoutId::CyrillicRu, freeink::ui::KeyboardLayoutId::CyrillicUk,
    freeink::ui::KeyboardLayoutId::CyrillicBe, freeink::ui::KeyboardLayoutId::CyrillicKk,
    freeink::ui::KeyboardLayoutId::HebrewIl,
};

class KeyboardLayoutSetTest : public ::testing::Test {
 protected:
  void SetUiLanguage(const Language lang) { I18n::getInstance().setLanguage(lang); }

  void TearDown() override { SetUiLanguage(Language::EN); }
};

}  // namespace

TEST_F(KeyboardLayoutSetTest, TableRowsMatchMainOrderIdsBitsAndNativeLabels) {
  EXPECT_EQ(keyboard_layouts::COUNT, 9u);
  for (uint8_t i = 0; i < keyboard_layouts::COUNT; ++i) {
    EXPECT_EQ(keyboard_layouts::ALL[i].id, kExpectedIds[i]) << "row " << static_cast<int>(i);
    EXPECT_EQ(keyboard_layouts::bitAt(i), static_cast<uint16_t>(1u << i)) << "row " << static_cast<int>(i);
    EXPECT_STREQ(keyboard_layouts::ALL[i].displayName, kExpectedDisplayNames[i]) << "row " << static_cast<int>(i);
  }
}

TEST_F(KeyboardLayoutSetTest, SavedMaskEnFrRuHeRoundTripsThroughEnabled) {
  SETTINGS.keyboardLayouts = kMaskEnFrRuHe;
  EXPECT_EQ(keyboard_layouts::enabled(), kMaskEnFrRuHe);
}

TEST_F(KeyboardLayoutSetTest, SavedAllNineMaskRoundTripsThroughEnabled) {
  SETTINGS.keyboardLayouts = kMaskAllNine;
  EXPECT_EQ(keyboard_layouts::enabled(), kMaskAllNine);
}

TEST_F(KeyboardLayoutSetTest, HighStaleBitsAreStrippedByAllBits) {
  SETTINGS.keyboardLayouts = static_cast<uint16_t>(0xFE00u | kMaskEnFrRuHe);
  EXPECT_EQ(keyboard_layouts::enabled(), kMaskEnFrRuHe);
}

TEST_F(KeyboardLayoutSetTest, HebrewOnlyMaskForcesLatinFallback) {
  SETTINGS.keyboardLayouts = keyboard_layouts::bitAt(8);
  const uint16_t expected = static_cast<uint16_t>(keyboard_layouts::bitAt(8) | keyboard_layouts::bitAt(0));
  EXPECT_EQ(keyboard_layouts::enabled(), expected);
}

TEST_F(KeyboardLayoutSetTest, FrenchOnlyMaskStartsOnAzerty) {
  SETTINGS.keyboardLayouts = keyboard_layouts::bitAt(1);
  EXPECT_EQ(keyboard_layouts::enabled(), keyboard_layouts::bitAt(1));
  EXPECT_EQ(keyboard_layouts::startingLayout(), freeink::ui::KeyboardLayoutId::AzertyFr);
}

TEST_F(KeyboardLayoutSetTest, EachLayoutBitIsIndividuallyEnableable) {
  for (uint8_t i = 0; i < keyboard_layouts::COUNT; ++i) {
    SETTINGS.keyboardLayouts = keyboard_layouts::bitAt(i);
    const uint16_t bit = keyboard_layouts::bitAt(i);
    const uint16_t expected = (i <= 3) ? bit : static_cast<uint16_t>(bit | keyboard_layouts::bitAt(0));
    EXPECT_EQ(keyboard_layouts::enabled(), expected) << "layout index " << static_cast<int>(i);
  }
}

TEST_F(KeyboardLayoutSetTest, UnconfiguredMaskDefaultsEnglishPlusUiLanguageEn) {
  SETTINGS.keyboardLayouts = 0;
  SetUiLanguage(Language::EN);
  const uint16_t expected = keyboard_layouts::bitAt(0);
  EXPECT_EQ(keyboard_layouts::enabled(), expected);
}

TEST_F(KeyboardLayoutSetTest, UnconfiguredMaskDefaultsEnglishPlusUiLanguageZhCn) {
  SETTINGS.keyboardLayouts = 0;
  SetUiLanguage(Language::ZH_CN);
  const uint16_t expected = static_cast<uint16_t>(keyboard_layouts::bitAt(0) | keyboard_layouts::bitAt(0));
  EXPECT_EQ(keyboard_layouts::enabled(), expected);
}

TEST_F(KeyboardLayoutSetTest, NextCyclesAllNineWhenAllEnabled) {
  SETTINGS.keyboardLayouts = kMaskAllNine;
  freeink::ui::KeyboardLayoutId current = keyboard_layouts::ALL[0].id;
  for (uint8_t step = 0; step < keyboard_layouts::COUNT; ++step) {
    current = keyboard_layouts::next(current);
    EXPECT_EQ(current, keyboard_layouts::ALL[(step + 1) % keyboard_layouts::COUNT].id);
  }
}

TEST_F(KeyboardLayoutSetTest, NextFromUnknownLayoutIdStartsFromTableIndexZero) {
  // Matches main: indexOf(unknown) == COUNT, so the scan starts at 0 — not at the
  // unknown id — and walks forward for the first enabled layout.
  SETTINGS.keyboardLayouts = keyboard_layouts::bitAt(1);
  EXPECT_EQ(keyboard_layouts::next(freeink::ui::KeyboardLayoutId::ArabicAr), freeink::ui::KeyboardLayoutId::AzertyFr);

  SETTINGS.keyboardLayouts = keyboard_layouts::bitAt(0);
  EXPECT_EQ(keyboard_layouts::next(freeink::ui::KeyboardLayoutId::ArabicAr), freeink::ui::KeyboardLayoutId::QwertyEn);

  // With EN+FR enabled, scan order from table index 0 visits FR (index 1) before EN (index 0).
  SETTINGS.keyboardLayouts = static_cast<uint16_t>(keyboard_layouts::bitAt(0) | keyboard_layouts::bitAt(1));
  EXPECT_EQ(keyboard_layouts::next(freeink::ui::KeyboardLayoutId::ArabicAr), freeink::ui::KeyboardLayoutId::AzertyFr);
}
