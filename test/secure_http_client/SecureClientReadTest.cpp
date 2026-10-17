#include <SecureClient.h>
#include <gtest/gtest.h>

namespace {

class SecureClientReadTest : public ::testing::Test {
 protected:
  void SetUp() override {
    client_.setHostTestConnected(true);
    client_.clearHostTestBufferedRead();
  }

  freeink::SecureClient client_;
};

}  // namespace

// Arduino Client::read() must not return 0 for "no byte yet" (WANT_READ) or EOF;
// callers treat 0 as NUL. Buffered read(&byte,1) may return 0; single-byte read()
// maps that to -1. Would fail edbe441f if read() returned 0 on WANT_READ.
TEST_F(SecureClientReadTest, SingleByteReadReturnsMinusOneOnWantRead) {
  client_.setHostTestBufferedRead(0);
  EXPECT_EQ(client_.read(), -1);
}

TEST_F(SecureClientReadTest, SingleByteReadReturnsMinusOneWhenDisconnected) {
  client_.setHostTestConnected(false);
  client_.clearHostTestBufferedRead();
  EXPECT_EQ(client_.read(), -1);
}
