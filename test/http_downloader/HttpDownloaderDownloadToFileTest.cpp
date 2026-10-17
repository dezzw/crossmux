#include <gtest/gtest.h>

#include <cstdio>
#include <string>

#include "HttpDownloader.h"

namespace {

std::string tempPath() {
  static int counter = 0;
  return "/tmp/crossmux-http-dl-test-" + std::to_string(++counter) + ".bin";
}

size_t fileSizeBytes(const std::string& path) {
  std::FILE* fp = std::fopen(path.c_str(), "rb");
  if (!fp) return 0;
  std::fseek(fp, 0, SEEK_END);
  const long sz = std::ftell(fp);
  std::fclose(fp);
  return sz >= 0 ? static_cast<size_t>(sz) : 0;
}

}  // namespace

class HttpDownloaderDownloadToFileTest : public ::testing::Test {
 protected:
  void TearDown() override { HttpDownloaderMockWolf::resetResponseStatus(); }
};

TEST_F(HttpDownloaderDownloadToFileTest, WolfMock2882BytesOkAndFileKept) {
  const std::string path = tempPath();
  const HttpDownloader::DownloadError result = HttpDownloader::downloadToFile("https://example.test/fonts.json", path);
  EXPECT_EQ(result, HttpDownloader::OK);
  EXPECT_EQ(fileSizeBytes(path), 2882u);
  std::remove(path.c_str());
}

// Exercises deliverWolfHttpBody (shared by real wolf GET callback and mock runGetWolf).
TEST_F(HttpDownloaderDownloadToFileTest, FetchUrl403NeverInvokesDataCallback) {
  HttpDownloaderMockWolf::setResponseStatus(403);
  size_t callbackInvocations = 0;
  EXPECT_FALSE(HttpDownloader::fetchUrl("https://example.test/denied.json",
                                        [&](const uint8_t*, size_t len) {
                                          callbackInvocations += (len > 0 ? 1u : 0u);
                                          return true;
                                        }));
  EXPECT_EQ(callbackInvocations, 0u);
  EXPECT_EQ(HttpDownloader::lastHttpStatusCode(), 403);
}
