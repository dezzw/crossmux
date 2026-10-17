#include <HalClock.h>
#include <I18n.h>
#include <gtest/gtest.h>

#include "HttpTransportFailure.h"
#include "HttpTransportFailureText.h"

TEST(HttpTransportFailureMap, MapsCertMemoryDateAndHandshake) {
  EXPECT_EQ(HttpTransportFailure::fromWolfError(HttpTransportFailure::kSecureClientCaLoadError),
            HttpTransportTypes::TransportFailure::TlsCert);
  EXPECT_EQ(HttpTransportFailure::fromWolfError(-125), HttpTransportTypes::TransportFailure::TlsMemory);
  EXPECT_EQ(HttpTransportFailure::fromWolfError(-150), HttpTransportTypes::TransportFailure::TlsDate);
  EXPECT_EQ(HttpTransportFailure::fromWolfError(-156), HttpTransportTypes::TransportFailure::TlsCert);
  EXPECT_EQ(HttpTransportFailure::fromWolfError(-999), HttpTransportTypes::TransportFailure::TlsHandshake);
  EXPECT_EQ(HttpTransportFailure::fromWolfError(0), HttpTransportTypes::TransportFailure::Connect);
}

TEST(HttpTransportFailureMap, MapsNamedWolfSslErrors) {
  EXPECT_EQ(HttpTransportFailure::fromWolfError(-232), HttpTransportTypes::TransportFailure::TlsCert);
  EXPECT_EQ(HttpTransportFailure::fromWolfError(-188), HttpTransportTypes::TransportFailure::TlsCert);
  EXPECT_EQ(HttpTransportFailure::fromWolfError(-322), HttpTransportTypes::TransportFailure::TlsCert);
  EXPECT_EQ(HttpTransportFailure::fromWolfError(-308), HttpTransportTypes::TransportFailure::Connect);
  EXPECT_EQ(HttpTransportFailure::fromWolfError(-343), HttpTransportTypes::TransportFailure::Connect);
  EXPECT_EQ(HttpTransportFailure::fromWolfError(-132), HttpTransportTypes::TransportFailure::TlsHandshake);
}

TEST(HttpTransportFailureClock, BlocksVerifiedTlsWhenClockInvalid) {
  HttpTransportFailure::resetVerifiedTlsClockGate();
  halClock.now = 0;
  halClock.syncNowResult = false;
  HttpTransportTypes::TransportFailure failure = HttpTransportTypes::TransportFailure::None;
  EXPECT_FALSE(HttpTransportFailure::ensureClockForVerifiedTls(failure));
  EXPECT_EQ(failure, HttpTransportTypes::TransportFailure::ClockNotSet);
}

TEST(HttpTransportFailureClock, SyncsOnceWhenClockInvalidThenAllowsTls) {
  HttpTransportFailure::resetVerifiedTlsClockGate();
  halClock.now = 0;
  halClock.syncNowResult = true;
  HttpTransportTypes::TransportFailure failure = HttpTransportTypes::TransportFailure::None;
  EXPECT_TRUE(HttpTransportFailure::ensureClockForVerifiedTls(failure));
  EXPECT_EQ(failure, HttpTransportTypes::TransportFailure::None);
  EXPECT_NE(halClock.now, 0);
}

TEST(HttpTransportFailureClock, LatchesClockNotSetUntilReset) {
  halClock.now = 0;
  halClock.syncNowResult = false;
  HttpTransportTypes::TransportFailure failure = HttpTransportTypes::TransportFailure::None;
  EXPECT_FALSE(HttpTransportFailure::ensureClockForVerifiedTls(failure));
  halClock.syncNowResult = true;
  EXPECT_FALSE(HttpTransportFailure::ensureClockForVerifiedTls(failure));
  EXPECT_EQ(failure, HttpTransportTypes::TransportFailure::ClockNotSet);
  HttpTransportFailure::resetVerifiedTlsClockGate();
  EXPECT_TRUE(HttpTransportFailure::ensureClockForVerifiedTls(failure));
}

TEST(HttpTransportFailureMapping, NetworkOnlyForConnect) {
  I18n::getInstance().setLanguage(Language::EN);
  EXPECT_STREQ(fontTransportFailureText(HttpTransportTypes::TransportFailure::Connect), "Network or DNS error");
}

TEST(HttpTransportFailureMapping, EmptyResponseUsesDedicatedString) {
  I18n::getInstance().setLanguage(Language::EN);
  EXPECT_STREQ(fontTransportFailureText(HttpTransportTypes::TransportFailure::EmptyResponse),
                "Empty response from server");
}

TEST(HttpTransportFailureMapping, IncompleteIsNotNetwork) {
  I18n::getInstance().setLanguage(Language::EN);
  EXPECT_STRNE(fontTransportFailureText(HttpTransportTypes::TransportFailure::Incomplete), "Network or DNS error");
}

TEST(HttpTransportFailureClock, DisconnectThenReconnectRetriesSntp) {
  halClock.now = 0;
  halClock.syncNowResult = false;
  HttpTransportTypes::TransportFailure failure = HttpTransportTypes::TransportFailure::None;
  EXPECT_FALSE(HttpTransportFailure::ensureClockForVerifiedTls(failure));
  halClock.syncNowResult = true;
  EXPECT_FALSE(HttpTransportFailure::ensureClockForVerifiedTls(failure));
  HttpTransportFailure::onWifiDisconnected();
  HttpTransportFailure::onWifiConnected();
  EXPECT_TRUE(HttpTransportFailure::ensureClockForVerifiedTls(failure));
  EXPECT_EQ(failure, HttpTransportTypes::TransportFailure::None);
}
