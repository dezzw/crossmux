"""SecureNet overlay copy + patch (shared by PlatformIO script and seed helper)."""

from __future__ import annotations

import shutil
from pathlib import Path

MARKER = "/* CrossMux SecureClient CA load check */"

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


def must_replace(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        raise RuntimeError(f"SecureClient patch missing anchor ({label})")
    return text.replace(old, new, 1)


def ensure_overlay(project_dir: Path) -> Path:
    source_dir = project_dir / "freeink-sdk/libs/network/SecureNet"
    overlay_dir = project_dir / "crossmux-patches/SecureNet"
    secure_client = overlay_dir / "src/SecureClient.cpp"
    if not source_dir.is_dir():
        raise RuntimeError(f"Missing SecureNet source at {source_dir}")
    if not overlay_dir.is_dir():
        shutil.copytree(source_dir, overlay_dir, symlinks=True)
        print(f"Copied SecureNet overlay: {overlay_dir.relative_to(project_dir)}")
        return overlay_dir
    stamp = overlay_dir / ".crossmux-source-stamp"
    src_version = source_dir / "src/SecureClient.cpp"
    stamp_text = src_version.read_text() if src_version.is_file() else ""
    if stamp.is_file() and stamp.read_text() == stamp_text and secure_client.is_file() and MARKER in secure_client.read_text():
        return overlay_dir
    shutil.rmtree(overlay_dir)
    shutil.copytree(source_dir, overlay_dir, symlinks=True)
    stamp.write_text(stamp_text)
    print(f"Refreshed SecureNet overlay from submodule")
    return overlay_dir


def apply_patches(project_dir: Path) -> None:
    overlay_dir = ensure_overlay(project_dir)
    secure_client = overlay_dir / "src/SecureClient.cpp"
    if secure_client.is_file() and MARKER in secure_client.read_text():
        return
    patch_secure_client_cpp(secure_client)
    patch_secure_client_h(overlay_dir / "include/SecureClient.h")
    patch_secure_http_h(overlay_dir / "include/SecureHttpClient.h")


def patch_secure_client_cpp(path: Path) -> None:
    text = path.read_text()
    if MARKER in text:
        text = text.split(MARKER, 1)[0].rstrip() + "\n"
    if "#include <cstring>" not in text:
        text = must_replace(
            text,
            "#endif\n\nnamespace freeink {",
            "#endif\n" + INCLUDES + "\nnamespace freeink {",
            "includes",
        )
    if "countPemCertificates" not in text:
        text = must_replace(
            text,
            "namespace {\n// Bridge wolfSSL",
            "namespace {\n" + COUNT_FN + "\n// Bridge wolfSSL",
            "countPemCertificates",
        )
    text = must_replace(
        text,
        "  stop();\n  const uint32_t timeoutMs = getTimeout();",
        "  stop();\n  _lastWolfError = 0;\n  const uint32_t timeoutMs = getTimeout();",
        "reset wolf error on connect",
    )
    text = must_replace(
        text,
        '    if (Serial) Serial.printf("[SecureClient] TCP connect failed (%s): %s:%u\\n", label, host, port);\n    return 0;',
        '    if (Serial) Serial.printf("[SecureClient] TCP connect failed (%s): %s:%u\\n", label, host, port);\n    _lastWolfError = 0;\n    return 0;',
        "tcp fail wolf error",
    )
    text = must_replace(
        text,
        '    if (Serial) Serial.printf("[SecureClient] CTX alloc failed (%s), free heap %u\\n", label, (unsigned)ESP.getFreeHeap());\n    _transport.stop();\n    return 0;',
        '    if (Serial) Serial.printf("[SecureClient] CTX alloc failed (%s), free heap %u\\n", label, (unsigned)ESP.getFreeHeap());\n    _lastWolfError = MEMORY_E;\n    _transport.stop();\n    return 0;',
        "ctx alloc",
    )
    old_load = """    wolfSSL_CTX_load_verify_buffer(ctx, reinterpret_cast<const unsigned char*>(_rootCA),
                                   strlen(_rootCA), WOLFSSL_FILETYPE_PEM);"""
    if old_load in text:
        text = text.replace(old_load, CA_LOAD_BLOCK.strip())
    elif "roots_in_pem" not in text:
        raise RuntimeError("SecureClient patch missing CA load anchor")
    text = must_replace(
        text,
        '    if (Serial) Serial.printf("[SecureClient] SSL alloc failed (%s), free heap %u\\n", label, (unsigned)ESP.getFreeHeap());\n    stop();\n    return 0;',
        '    if (Serial) Serial.printf("[SecureClient] SSL alloc failed (%s), free heap %u\\n", label, (unsigned)ESP.getFreeHeap());\n    _lastWolfError = MEMORY_E;\n    stop();\n    return 0;',
        "ssl alloc",
    )
    text = must_replace(
        text,
        '      if (Serial) Serial.printf("[SecureClient] wolfSSL_connect failed (%s): %d\\n", label, err);\n      stop();\n      return 0;',
        '      if (Serial) Serial.printf("[SecureClient] wolfSSL_connect failed (%s): %d\\n", label, err);\n      _lastWolfError = err;\n      stop();\n      return 0;',
        "connect fail",
    )
    text = must_replace(
        text,
        "      stop();\n      return 0;\n    }\n    delay(5);",
        "      _lastWolfError = err;\n      stop();\n      return 0;\n    }\n    delay(5);",
        "handshake timeout",
    )
    text = text.rstrip() + f"\n\n{MARKER}\n"
    path.write_text(text)


def patch_secure_client_h(path: Path) -> None:
    text = path.read_text()
    if "lastWolfErrorCode" in text:
        return
    text = must_replace(
        text,
        "  static bool tls13Available();\n\n private:",
        """  static bool tls13Available();

  // Must match src/network/WolfSslTransportErrors.h (kSecureClientCaLoadError).
  static constexpr int kCaLoadError = -2000;
  int lastWolfErrorCode() const { return _lastWolfError; }

 private:""",
        "SecureClient.h lastWolfError",
    )
    text = must_replace(
        text,
        "  bool _connected = false;\n};",
        "  bool _connected = false;\n  int _lastWolfError = 0;\n};",
        "SecureClient.h member",
    )
    path.write_text(text)


def patch_secure_http_h(path: Path) -> None:
    text = path.read_text()
    if "lastWolfErrorCode" in text:
        return
    text = must_replace(
        text,
        "  static bool tls13Available() { return SecureClient::tls13Available(); }\n\n private:",
        "  static bool tls13Available() { return SecureClient::tls13Available(); }\n\n"
        "  int lastWolfErrorCode() const { return _secure.lastWolfErrorCode(); }\n\n private:",
        "SecureHttpClient.h",
    )
    path.write_text(text)
