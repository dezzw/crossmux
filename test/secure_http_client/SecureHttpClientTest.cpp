#define CROSSPOINT_SECURE_HTTP_CLIENT_TEST 1

#include <gtest/gtest.h>

#include <SecureHttpClient.h>

#include <string>

#include "MockHttpClient.h"

namespace {

std::string httpResponse(int status, const std::string& headers, const std::string& body) {
  std::string out = "HTTP/1.1 " + std::to_string(status) + " OK\r\n" + headers;
  if (!headers.empty() && headers.back() != '\n') out += "\r\n";
  out += "\r\n";
  out += body;
  return out;
}

}  // namespace

TEST(SecureHttpClientBody, WantReadMidHeaderStillParsesContentLength) {
  MockHttpClient mock;
  mock.armWantReadAfterAvailable(2);
  mock.enqueueResponse(httpResponse(200, "Content-Length: 4\r\n", "data"));

  freeink::SecureHttpClient http;
  ASSERT_TRUE(http.begin("http://example.test/file"));
  http.setTestTransport(&mock);

  size_t received = 0;
  const int status = http.GET([&](const uint8_t*, size_t len) {
    received += len;
    return true;
  });

  EXPECT_EQ(status, 200);
  EXPECT_TRUE(http.responseComplete());
  EXPECT_EQ(received, 4u);
}

TEST(SecureHttpClientBody, PeerCloseBeforeBodyWithContentLengthFails) {
  MockHttpClient mock;
  mock.enqueueResponse("HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\n");

  freeink::SecureHttpClient http;
  ASSERT_TRUE(http.begin("http://example.test/file"));
  http.setTestTransport(&mock);

  const int status = http.GET([](const uint8_t*, size_t) { return true; });
  EXPECT_EQ(status, 200);
  EXPECT_FALSE(http.responseComplete());
  EXPECT_EQ(http.bodyBytesDelivered(), 0u);
}

TEST(SecureHttpClientBody, EmptyBodyWithoutExplicitZeroLengthFails) {
  MockHttpClient mock;
  mock.enqueueResponse("HTTP/1.1 200 OK\r\n\r\n");

  freeink::SecureHttpClient http;
  ASSERT_TRUE(http.begin("http://example.test/file"));
  http.setTestTransport(&mock);

  const int status = http.GET([](const uint8_t*, size_t) { return true; });
  EXPECT_EQ(status, 200);
  EXPECT_FALSE(http.responseComplete());
}

TEST(SecureHttpClientBody, ExplicitContentLengthZeroSucceeds) {
  MockHttpClient mock;
  mock.enqueueResponse("HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");

  freeink::SecureHttpClient http;
  ASSERT_TRUE(http.begin("http://example.test/file"));
  http.setTestTransport(&mock);

  const int status = http.GET([](const uint8_t*, size_t) { return true; });
  EXPECT_EQ(status, 200);
  EXPECT_TRUE(http.responseComplete());
  EXPECT_EQ(http.bodyBytesDelivered(), 0u);
}

TEST(SecureHttpClientBody, ChunkedBodyDelivered) {
  MockHttpClient mock;
  mock.enqueueResponse(
      "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
      "4\r\n"
      "test\r\n"
      "0\r\n\r\n");

  freeink::SecureHttpClient http;
  ASSERT_TRUE(http.begin("http://example.test/file"));
  http.setTestTransport(&mock);

  std::string body;
  const int status = http.GET([&](const uint8_t* data, size_t len) {
    body.append(reinterpret_cast<const char*>(data), len);
    return true;
  });
  EXPECT_EQ(status, 200);
  EXPECT_TRUE(http.responseComplete());
  EXPECT_EQ(body, "test");
}

TEST(SecureHttpClientBody, RedirectBodyDrainedWithoutDeliveringToCaller) {
  MockHttpClient mock;
  mock.enqueueResponse(httpResponse(302, "Location: /final\r\nContent-Length: 5\r\n", "junk\n"));

  freeink::SecureHttpClient http;
  ASSERT_TRUE(http.begin("http://example.test/start"));
  http.setTestTransport(&mock);
  http.setDrainRedirectResponseBodies(true);

  size_t received = 0;
  const int status = http.GET([&](const uint8_t*, size_t len) {
    received += len;
    return true;
  });
  EXPECT_EQ(status, 302);
  EXPECT_TRUE(http.responseComplete());
  EXPECT_EQ(received, 0u);
}

TEST(SecureHttpClientBody, Final200BodyCountedAfterRedirectDrain) {
  MockHttpClient redirectMock;
  redirectMock.enqueueResponse(httpResponse(302, "Location: /final\r\nContent-Length: 3\r\n", "abc"));

  freeink::SecureHttpClient http;
  ASSERT_TRUE(http.begin("http://example.test/start"));
  http.setTestTransport(&redirectMock);
  http.setDrainRedirectResponseBodies(true);

  size_t received = 0;
  int status = http.GET([&](const uint8_t*, size_t len) {
    received += len;
    return true;
  });
  EXPECT_EQ(status, 302);
  EXPECT_EQ(received, 0u);

  MockHttpClient finalMock;
  finalMock.enqueueResponse(httpResponse(200, "Content-Length: 5\r\n", "hello"));
  ASSERT_TRUE(http.begin("http://example.test/final"));
  http.setTestTransport(&finalMock);
  status = http.GET([&](const uint8_t*, size_t len) {
    received += len;
    return true;
  });
  EXPECT_EQ(status, 200);
  EXPECT_TRUE(http.responseComplete());
  EXPECT_EQ(received, 5u);
}
