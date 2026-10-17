#pragma once

#include <cstddef>
#include <cstdint>

struct IPAddress {};

class Print {
 public:
  virtual ~Print() = default;
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t* buffer, size_t size) {
    size_t n = 0;
    while (size--) n += write(*buffer++);
    return n;
  }
};

class Stream : public Print {
 public:
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int read(uint8_t* buf, size_t size) = 0;
  virtual int peek() = 0;
  virtual void flush() = 0;
};

class Client : public Stream {
 public:
  virtual int connect(IPAddress ip, uint16_t port) = 0;
  virtual int connect(const char* host, uint16_t port) = 0;
  virtual void stop() = 0;
  virtual uint8_t connected() = 0;
  virtual operator bool() { return connected() != 0; }
};
