"""SecureNet overlay copy + patch (shared by PlatformIO script and seed helper)."""

from __future__ import annotations

import hashlib
import shutil
import subprocess
from pathlib import Path

MARKER = "/* CrossMux SecureClient CA load check */"
SDK_SOURCES = (
    "src/SecureClient.cpp",
    "include/SecureClient.h",
    "include/SecureHttpClient.h",
)
OVERLAY_SOURCES = (
    "src/SecureClient.cpp",
    "include/SecureClient.h",
    "include/SecureHttpClient.h",
)

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


def compute_sdk_stamp(source_dir: Path) -> str:
    digest = hashlib.sha256()
    for rel in SDK_SOURCES:
        digest.update((source_dir / rel).read_bytes())
    return digest.hexdigest()


def compute_file_stamp(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_stamp_file(stamp_path: Path) -> tuple[str, str] | None:
    if not stamp_path.is_file():
        return None
    lines = [line.strip() for line in stamp_path.read_text().splitlines() if line.strip()]
    if len(lines) < 2:
        return None
    return lines[0], lines[1]


def write_stamp_file(stamp_path: Path, sdk_stamp: str, patched_cpp_stamp: str) -> None:
    stamp_path.write_text(f"{sdk_stamp}\n{patched_cpp_stamp}\n")


def format_overlay_sources(overlay_dir: Path) -> None:
    clang = shutil.which("clang-format-21") or shutil.which("clang-format")
    if not clang:
        return
    for rel in OVERLAY_SOURCES:
        path = overlay_dir / rel
        if path.is_file():
            subprocess.run([clang, "-i", str(path)], check=False)


def ensure_overlay(project_dir: Path) -> Path:
    source_dir = project_dir / "freeink-sdk/libs/network/SecureNet"
    overlay_dir = project_dir / "crossmux-patches/SecureNet"
    secure_client = overlay_dir / "src/SecureClient.cpp"
    stamp_path = overlay_dir / ".crossmux-source-stamp"
    if not source_dir.is_dir():
        raise RuntimeError(f"Missing SecureNet source at {source_dir}")
    sdk_stamp = compute_sdk_stamp(source_dir)
    stamps = read_stamp_file(stamp_path)
    if (
        overlay_dir.is_dir()
        and secure_client.is_file()
        and MARKER in secure_client.read_text()
        and stamps is not None
        and stamps[0] == sdk_stamp
        and stamps[1] == compute_file_stamp(secure_client)
    ):
        return overlay_dir
    if overlay_dir.is_dir():
        shutil.rmtree(overlay_dir)
    shutil.copytree(source_dir, overlay_dir, symlinks=True)
    print(f"Refreshed SecureNet overlay from submodule")
    return overlay_dir


def apply_patches(project_dir: Path) -> None:
    overlay_dir = ensure_overlay(project_dir)
    secure_client = overlay_dir / "src/SecureClient.cpp"
    stamp_path = overlay_dir / ".crossmux-source-stamp"
    source_dir = project_dir / "freeink-sdk/libs/network/SecureNet"
    sdk_stamp = compute_sdk_stamp(source_dir)

    if secure_client.is_file() and MARKER in secure_client.read_text():
        stamps = read_stamp_file(stamp_path)
        if stamps and stamps[0] == sdk_stamp and stamps[1] == compute_file_stamp(secure_client):
            return

    patch_secure_client_cpp(secure_client)
    patch_secure_client_h(overlay_dir / "include/SecureClient.h")
    patch_secure_http_h(overlay_dir / "include/SecureHttpClient.h")
    format_overlay_sources(overlay_dir)
    write_stamp_file(stamp_path, sdk_stamp, compute_file_stamp(secure_client))


def check_overlay(project_dir: Path) -> None:
    source_dir = project_dir / "freeink-sdk/libs/network/SecureNet"
    overlay_dir = project_dir / "crossmux-patches/SecureNet"
    stamp_path = overlay_dir / ".crossmux-source-stamp"
    secure_client = overlay_dir / "src/SecureClient.cpp"
    if not overlay_dir.is_dir():
        raise SystemExit(f"::error::Missing overlay at {overlay_dir}")
    sdk_stamp = compute_sdk_stamp(source_dir)
    stamps = read_stamp_file(stamp_path)
    if stamps is None:
        raise SystemExit(f"::error::Missing or invalid stamp file at {stamp_path}")
    if stamps[0] != sdk_stamp:
        raise SystemExit(
            f"::error::SecureNet overlay stale (SDK changed). Run: python3 scripts/seed_securenet_overlay.py"
        )
    if MARKER not in secure_client.read_text():
        raise SystemExit("::error::SecureNet overlay missing CrossMux patch marker")
    if stamps[1] != compute_file_stamp(secure_client):
        raise SystemExit(
            "::error::SecureNet overlay SecureClient.cpp drift. Run: python3 scripts/seed_securenet_overlay.py"
        )


def patch_secure_client_cpp(path: Path) -> None:
    text = path.read_text()
    if MARKER in text:
        text = text.split(MARKER, 1)[0].lstrip("\n").rstrip() + "\n"
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
        text = text.replace(old_load, CA_LOAD_BLOCK.lstrip("\n").rstrip())
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