from pathlib import Path

Import("env")

PROJECT_DIR = Path(env.subst("$PROJECT_DIR"))
MARKER = "/* CrossMux SecureClient CA load check */"

SECURE_CLIENT = PROJECT_DIR / "freeink-sdk/libs/network/SecureNet/src/SecureClient.cpp"
SECURE_CLIENT_H = PROJECT_DIR / "freeink-sdk/libs/network/SecureNet/include/SecureClient.h"
SECURE_HTTP_H = PROJECT_DIR / "freeink-sdk/libs/network/SecureNet/include/SecureHttpClient.h"

INCLUDES = """
#include <cstring>
#if defined(FREEINK_NET_WOLFSSL)
#include <wolfssl/wolfcrypt/error-crypt.h>
#endif
"""

COUNT_FN = """
size_t countPemCertificates(const char* pem) {
  if (!pem) return 0;
  size_t count = 0;
  const char* cursor = pem;
  while ((cursor = strstr(cursor, "-----BEGIN CERTIFICATE-----")) != nullptr) {
    ++count;
    cursor += strlen("-----BEGIN CERTIFICATE-----");
  }
  return count;
}
"""

CA_LOAD_BLOCK = """
    const size_t rootPemLen = strlen(_rootCA);
    const size_t rootCount = countPemCertificates(_rootCA);
    const int loadRc =
        wolfSSL_CTX_load_verify_buffer(ctx, reinterpret_cast<const unsigned char*>(_rootCA),
                                       rootPemLen, WOLFSSL_FILETYPE_PEM);
    if (Serial) {
      Serial.printf("[SecureClient] CA load (%s): rc=%d roots_in_pem=%u pem_bytes=%u\\n", label, loadRc,
                    static_cast<unsigned>(rootCount), static_cast<unsigned>(rootPemLen));
    }
    if (loadRc != WOLFSSL_SUCCESS) {
      if (Serial) Serial.printf("[SecureClient] CA load failed (%s): rc=%d\\n", label, loadRc);
      _lastWolfError = kCaLoadError;
      stop();
      return 0;
    }
"""


def patch_secure_client_cpp(path: Path) -> None:
    text = path.read_text()
    if MARKER in text:
        return
    if "#include <cstring>" not in text:
        text = text.replace("#endif\n\nnamespace freeink {", "#endif\n" + INCLUDES + "\nnamespace freeink {")
    if "countPemCertificates" not in text:
        text = text.replace("namespace {\n// Bridge wolfSSL", "namespace {\n" + COUNT_FN + "\n// Bridge wolfSSL")
    text = text.replace(
        "  stop();\n  const uint32_t timeoutMs = getTimeout();",
        "  stop();\n  _lastWolfError = 0;\n  const uint32_t timeoutMs = getTimeout();",
    )
    text = text.replace(
        '    if (Serial) Serial.printf("[SecureClient] TCP connect failed (%s): %s:%u\\n", label, host, port);\n    return 0;',
        '    if (Serial) Serial.printf("[SecureClient] TCP connect failed (%s): %s:%u\\n", label, host, port);\n    _lastWolfError = 0;\n    return 0;',
    )
    text = text.replace(
        '    if (Serial) Serial.printf("[SecureClient] CTX alloc failed (%s), free heap %u\\n", label, (unsigned)ESP.getFreeHeap());\n    _transport.stop();\n    return 0;',
        '    if (Serial) Serial.printf("[SecureClient] CTX alloc failed (%s), free heap %u\\n", label, (unsigned)ESP.getFreeHeap());\n    _lastWolfError = MEMORY_E;\n    _transport.stop();\n    return 0;',
    )
    old_load = """    wolfSSL_CTX_load_verify_buffer(ctx, reinterpret_cast<const unsigned char*>(_rootCA),
                                   strlen(_rootCA), WOLFSSL_FILETYPE_PEM);"""
    if old_load in text:
        text = text.replace(old_load, CA_LOAD_BLOCK.strip())
    text = text.replace(
        '    if (Serial) Serial.printf("[SecureClient] SSL alloc failed (%s), free heap %u\\n", label, (unsigned)ESP.getFreeHeap());\n    stop();\n    return 0;',
        '    if (Serial) Serial.printf("[SecureClient] SSL alloc failed (%s), free heap %u\\n", label, (unsigned)ESP.getFreeHeap());\n    _lastWolfError = MEMORY_E;\n    stop();\n    return 0;',
    )
    text = text.replace(
        '      if (Serial) Serial.printf("[SecureClient] wolfSSL_connect failed (%s): %d\\n", label, err);\n      stop();\n      return 0;',
        '      if (Serial) Serial.printf("[SecureClient] wolfSSL_connect failed (%s): %d\\n", label, err);\n      _lastWolfError = err;\n      stop();\n      return 0;',
    )
    text = text.replace(
        "      stop();\n      return 0;\n    }\n    delay(5);",
        "      _lastWolfError = err;\n      stop();\n      return 0;\n    }\n    delay(5);",
        1,
    )
    text = text.rstrip() + f"\n\n{MARKER}\n"
    path.write_text(text)
    print(f"Patched SecureClient: {path.relative_to(PROJECT_DIR)}")


def patch_secure_client_h(path: Path) -> None:
    text = path.read_text()
    if "lastWolfErrorCode" in text:
        return
    text = text.replace(
        "  static bool tls13Available();\n\n private:",
        """  static bool tls13Available();

  static constexpr int kCaLoadError = -2000;
  int lastWolfErrorCode() const { return _lastWolfError; }

 private:""",
    )
    text = text.replace("  bool _connected = false;\n};", "  bool _connected = false;\n  int _lastWolfError = 0;\n};")
    path.write_text(text)


def patch_secure_http_h(path: Path) -> None:
    text = path.read_text()
    if "lastWolfErrorCode" in text:
        return
    text = text.replace(
        "  static bool tls13Available() { return SecureClient::tls13Available(); }\n\n private:",
        "  static bool tls13Available() { return SecureClient::tls13Available(); }\n\n"
        "  int lastWolfErrorCode() const { return _secure.lastWolfErrorCode(); }\n\n private:",
    )
    path.write_text(text)


if SECURE_CLIENT.is_file():
    patch_secure_client_cpp(SECURE_CLIENT)
    patch_secure_client_h(SECURE_CLIENT_H)
    patch_secure_http_h(SECURE_HTTP_H)
