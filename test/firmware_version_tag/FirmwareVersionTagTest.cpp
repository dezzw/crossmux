#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "src/network/FirmwareVersionTag.h"

TEST(FirmwareVersionTag, ScannerFindsEmbeddedVersion) {
  const char* expected = firmware_version_tag::versionString();
  const size_t expectedLen = firmware_version_tag::versionStringLen();
  ASSERT_GT(expectedLen, 0u);

  firmware_version_tag::Scanner scanner;
  scanner.setExpected(expected, expectedLen);
  scanner.feed(reinterpret_cast<const uint8_t*>(firmware_version_tag::TAG), strlen(firmware_version_tag::TAG));
  scanner.markStreamComplete();
  EXPECT_TRUE(scanner.matchConfirmed());
  EXPECT_FALSE(scanner.mismatch());
  EXPECT_EQ(std::string(scanner.foundVersion()), std::string(expected, expectedLen));
}

TEST(FirmwareVersionTag, RejectsMismatchedManifestVersion) {
  const char* expected = firmware_version_tag::versionString();
  const size_t expectedLen = firmware_version_tag::versionStringLen();

  firmware_version_tag::Scanner scanner;
  scanner.setExpected("9.9.9-wrong-version-marker", 26);
  scanner.feed(reinterpret_cast<const uint8_t*>(firmware_version_tag::TAG), strlen(firmware_version_tag::TAG));
  scanner.markStreamComplete();
  EXPECT_TRUE(scanner.mismatch());
}
