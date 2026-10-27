#include <gtest/gtest.h>

#include "lib/Epub/Epub/hyphenation/LanguageRegistry.h"

TEST(LanguageRegistryAllLangs, NonEnglishHyphenatorsAreRegistered) {
  EXPECT_NE(getLanguageHyphenatorForPrimaryTag("en"), nullptr);
  EXPECT_NE(getLanguageHyphenatorForPrimaryTag("de"), nullptr);
  EXPECT_NE(getLanguageHyphenatorForPrimaryTag("fr"), nullptr);
  EXPECT_GE(getLanguageEntries().size, 2u);
}
