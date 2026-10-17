#pragma once

#include <Client.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// In-memory Client for SecureHttpClient host tests.
class MockHttpClient : public Client {
 public:
  int connect(const char*, uint16_t) override { return 1; }
  int connect(IPAddress, uint16_t) override { return 1; }

  void enqueueResponse(const std::string& rawHttp) { response_.append(rawHttp); }

  void setConnected(bool connected) { connected_ = connected; }

  void armWantReadAfterAvailable(int count) { wantReadAfterAvailable_ = count; }

  size_t write(uint8_t b) override { return write(&b, 1); }

  size_t write(const uint8_t* buf, size_t size) override {
    lastRequest_.append(reinterpret_cast<const char*>(buf), size);
    return size;
  }

  int available() override {
    if (responseOffset_ >= response_.size()) return 0;
    return static_cast<int>(response_.size() - responseOffset_);
  }

  int read(uint8_t* buf, size_t size) override {
    if (wantReadAfterAvailable_ > 0 && available() > 0) {
      --wantReadAfterAvailable_;
      return 0;
    }
    if (responseOffset_ >= response_.size()) {
      connected_ = false;
      return 0;
    }
    const size_t n = std::min(size, response_.size() - responseOffset_);
    for (size_t i = 0; i < n; ++i) buf[i] = static_cast<uint8_t>(response_[responseOffset_++]);
    if (responseOffset_ >= response_.size()) connected_ = false;
    return static_cast<int>(n);
  }

  int read() override {
    uint8_t b = 0;
    return read(&b, 1) == 1 ? static_cast<int>(b) : -1;
  }

  int peek() override { return -1; }
  void flush() override {}
  void stop() override {
    connected_ = false;
    responseOffset_ = 0;
    response_.clear();
  }
  uint8_t connected() override { return connected_ ? 1 : 0; }
  operator bool() override { return connected_ != 0; }

  const std::string& lastRequest() const { return lastRequest_; }

 private:
  std::string response_;
  size_t responseOffset_ = 0;
  std::string lastRequest_;
  bool connected_ = true;
  int wantReadAfterAvailable_ = 0;
};
