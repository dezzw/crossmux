#include <wolfssl/error-ssl.h>
#include <wolfssl/ssl.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace {

std::string readFile(const char* path) {
  std::ifstream in(path);
  if (!in) return {};
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

int tcpConnect(const char* host, uint16_t port) {
  addrinfo hints {};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo* res = nullptr;
  char portBuf[8];
  std::snprintf(portBuf, sizeof(portBuf), "%u", port);
  if (getaddrinfo(host, portBuf, &hints, &res) != 0 || !res) return -1;
  int sock = -1;
  for (addrinfo* p = res; p; p = p->ai_next) {
    sock = static_cast<int>(socket(p->ai_family, p->ai_socktype, p->ai_protocol));
    if (sock < 0) continue;
    if (connect(sock, p->ai_addr, p->ai_addrlen) == 0) break;
    close(sock);
    sock = -1;
  }
  freeaddrinfo(res);
  return sock;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    std::fprintf(stderr, "usage: %s <host> <ca-bundle.pem>\n", argv[0]);
    return 2;
  }
  const char* host = argv[1];
  const char* caPath = argv[2];
  const std::string caPem = readFile(caPath);
  if (caPem.empty()) {
    std::fprintf(stderr, "missing CA bundle: %s\n", caPath);
    return 1;
  }
  if (wolfSSL_Init() != WOLFSSL_SUCCESS) return 1;

  WOLFSSL_CTX* ctx = wolfSSL_CTX_new(wolfTLS_client_method());
  if (!ctx) {
    wolfSSL_Cleanup();
    return 1;
  }
  wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_PEER, nullptr);
  if (wolfSSL_CTX_load_verify_buffer(ctx, reinterpret_cast<const unsigned char*>(caPem.data()),
                                     static_cast<long>(caPem.size()), WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
    std::fprintf(stderr, "CA load failed\n");
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    return 1;
  }

  const int sock = tcpConnect(host, 443);
  if (sock < 0) {
    std::fprintf(stderr, "TCP connect failed: %s:443\n", host);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    return 1;
  }

  WOLFSSL* ssl = wolfSSL_new(ctx);
  if (!ssl) {
    close(sock);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    return 1;
  }
  wolfSSL_set_fd(ssl, sock);
  if (wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, host, static_cast<unsigned short>(std::strlen(host))) != WOLFSSL_SUCCESS) {
    std::fprintf(stderr, "SNI failed for %s\n", host);
    wolfSSL_free(ssl);
    close(sock);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    return 1;
  }

  const int rc = wolfSSL_connect(ssl);
  if (rc != WOLFSSL_SUCCESS) {
    const int err = wolfSSL_get_error(ssl, rc);
    std::fprintf(stderr, "TLS handshake failed for %s: rc=%d err=%d\n", host, rc, err);
    wolfSSL_free(ssl);
    close(sock);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    return 1;
  }

  wolfSSL_free(ssl);
  close(sock);
  wolfSSL_CTX_free(ctx);
  wolfSSL_Cleanup();
  std::printf("OK %s\n", host);
  return 0;
}
