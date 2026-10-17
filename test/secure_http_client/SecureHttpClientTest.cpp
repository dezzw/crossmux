#define CROSSPOINT_SECURE_HTTP_CLIENT_TEST 1

#include <SecureHttpClient.h>
#include <gtest/gtest.h>

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
