#include <gtest/gtest.h>

#include "GfxOutsideRangeLogBudget.h"

TEST(GfxOutsideRangeLogBudget, LogsFirstPixelOnlyPerFrame) {
  GfxOutsideRangeLogBudget budget;
  EXPECT_TRUE(budget.noteOutsideRange());
  EXPECT_FALSE(budget.noteOutsideRange());
  EXPECT_FALSE(budget.noteOutsideRange());
  EXPECT_EQ(budget.suppressedThisFrame(), 2U);

  budget.resetFrame();
  EXPECT_TRUE(budget.noteOutsideRange());
  EXPECT_EQ(budget.suppressedThisFrame(), 0U);
}
