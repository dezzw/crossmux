#include <KeyboardLayoutSet.h>
#include <gtest/gtest.h>

#include "CrossPointSettings.h"

namespace {

constexpr uint16_t kMaskEnFrRuHe =
    keyboard_layouts::bitAt(0) | keyboard_layouts::bitAt(1) | keyboard_layouts::bitAt(4) | keyboard_layouts::bitAt(8);
constexpr uint16_t kMaskAllNine = static_cast<uint16_t>((1u << keyboard_layouts::COUNT) - 1u);

}  // namespace

TEST(KeyboardLayoutSetTest, TableHasNineLayoutsInPersistedOrder) {
  EXPECT_EQ(keyboard_layouts::COUNT, 9u);
  EXPECT_EQ(keyboard_layouts::ALL[1].id, freeink::ui::KeyboardLayoutId::AzertyFr);
  EXPECT_EQ(keyboard_layouts::ALL[4].id, freeink::ui::KeyboardLayoutId::CyrillicRu);
  EXPECT_EQ(keyboard_layouts::ALL[8].id, freeink::ui::KeyboardLayoutId::HebrewIl);
}

TEST(KeyboardLayoutSetTest, SavedMaskEnFrRuHeRoundTripsThroughEnabled) {
  SETTINGS.keyboardLayouts = kMaskEnFrRuHe;
  EXPECT_EQ(keyboard_layouts::enabled(), kMaskEnFrRuHe);
}

TEST(KeyboardLayoutSetTest, SavedAllNineMaskRoundTripsThroughEnabled) {
  SETTINGS.keyboardLayouts = kMaskAllNine;
  EXPECT_EQ(keyboard_layouts::enabled(), kMaskAllNine);
}

TEST(KeyboardLayoutSetTest, HebrewOnlyMaskForcesLatinFallback) {
  SETTINGS.keyboardLayouts = keyboard_layouts::bitAt(8);
  const uint16_t expected = static_cast<uint16_t>(keyboard_layouts::bitAt(8) | keyboard_layouts::bitAt(0));
  EXPECT_EQ(keyboard_layouts::enabled(), expected);
}

TEST(KeyboardLayoutSetTest, FrenchOnlyMaskStartsOnAzerty) {
  SETTINGS.keyboardLayouts = keyboard_layouts::bitAt(1);
  EXPECT_EQ(keyboard_layouts::enabled(), keyboard_layouts::bitAt(1));
  EXPECT_EQ(keyboard_layouts::startingLayout(), freeink::ui::KeyboardLayoutId::AzertyFr);
}

TEST(KeyboardLayoutSetTest, EachLayoutBitIsIndividuallyEnableable) {
  for (uint8_t i = 0; i < keyboard_layouts::COUNT; ++i) {
    SETTINGS.keyboardLayouts = keyboard_layouts::bitAt(i);
    const uint16_t bit = keyboard_layouts::bitAt(i);
    const uint16_t expected = (i <= 3) ? bit : static_cast<uint16_t>(bit | keyboard_layouts::bitAt(0));
    EXPECT_EQ(keyboard_layouts::enabled(), expected) << "layout index " << static_cast<int>(i);
  }
}

TEST(KeyboardLayoutSetTest, NextCyclesAllNineWhenAllEnabled) {
  SETTINGS.keyboardLayouts = kMaskAllNine;
  freeink::ui::KeyboardLayoutId current = keyboard_layouts::ALL[0].id;
  for (uint8_t step = 0; step < keyboard_layouts::COUNT; ++step) {
    current = keyboard_layouts::next(current);
    EXPECT_EQ(current, keyboard_layouts::ALL[(step + 1) % keyboard_layouts::COUNT].id);
  }
}
