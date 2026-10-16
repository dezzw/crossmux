#include <gtest/gtest.h>

#include <HalClock.h>

#include "HttpTransportFailure.h"

TEST(HttpTransportFailureMap, MapsCertMemoryDateAndHandshake) {
  EXPECT_EQ(HttpTransportFailure::fromWolfError(HttpTransportFailure::kSecureClientCaLoadError),
            HttpTransportTypes::TransportFailure::TlsCert);
  EXPECT_EQ(HttpTransportFailure::fromWolfError(-125), HttpTransportTypes::TransportFailure::TlsMemory);
  EXPECT_EQ(HttpTransportFailure::fromWolfError(-150), HttpTransportTypes::TransportFailure::TlsDate);
  EXPECT_EQ(HttpTransportFailure::fromWolfError(-156), HttpTransportTypes::TransportFailure::TlsCert);
  EXPECT_EQ(HttpTransportFailure::fromWolfError(-999), HttpTransportTypes::TransportFailure::TlsHandshake);
  EXPECT_EQ(HttpTransportFailure::fromWolfError(0), HttpTransportTypes::TransportFailure::Connect);
}

TEST(HttpTransportFailureClock, BlocksVerifiedTlsWhenClockInvalid) {
  halClock.now = 0;
  halClock.syncNowResult = false;
  HttpTransportTypes::TransportFailure failure = HttpTransportTypes::TransportFailure::None;
  EXPECT_FALSE(HttpTransportFailure::ensureClockForVerifiedTls(failure));
  EXPECT_EQ(failure, HttpTransportTypes::TransportFailure::ClockNotSet);
}

TEST(HttpTransportFailureClock, SyncsOnceWhenClockInvalidThenAllowsTls) {
  halClock.now = 0;
  halClock.syncNowResult = true;
  HttpTransportTypes::TransportFailure failure = HttpTransportTypes::TransportFailure::None;
  EXPECT_TRUE(HttpTransportFailure::ensureClockForVerifiedTls(failure));
  EXPECT_EQ(failure, HttpTransportTypes::TransportFailure::None);
  EXPECT_NE(halClock.now, 0);
}
