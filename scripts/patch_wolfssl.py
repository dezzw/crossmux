import sys
from pathlib import Path

Import("env")

PROJECT_DIR = Path(env.subst("$PROJECT_DIR"))
MARKER = "/* CrossPoint wolfSSL compatibility overrides */"
OVERRIDES = f"""

{MARKER}
#ifndef WOLFSSL_SHA384
#define WOLFSSL_SHA384
#endif
#ifndef WOLFSSL_SHA512
#define WOLFSSL_SHA512
#endif
#ifndef WOLFSSL_SP_384
#define WOLFSSL_SP_384
#endif
#ifndef WOLFSSL_ALT_CERT_CHAINS
#define WOLFSSL_ALT_CERT_CHAINS
#endif
#undef NO_DH
#ifndef HAVE_FFDHE_2048
#define HAVE_FFDHE_2048
#endif
/* MEMFIX-PORT: 8192 handles up to RSA-4096 keys (the public-CA maximum,
   ISRG Root X1 included) with half the per-bignum heap of 16384: with
   WOLFSSL_SMALL_STACK each fast-math temp is FP_MAX_BITS/8 * 2 bytes on the
   heap, and TLS cert verification allocates dozens at once. */
#undef FP_MAX_BITS
#define FP_MAX_BITS 8192
#ifndef WOLFSSL_SHA384
#error "CrossMux wolfSSL overrides require WOLFSSL_SHA384"
#endif
"""


def patch_user_settings(path: Path) -> None:
    text = path.read_text()
    if MARKER in text:
        text = text.split(MARKER, 1)[0].rstrip()
    path.write_text(text + OVERRIDES + "\n")
    print(f"Patched wolfSSL settings: {path.relative_to(PROJECT_DIR)}")


libdeps_root = PROJECT_DIR / ".pio/libdeps"
matches = list(libdeps_root.glob("*/Arduino-wolfSSL/src/user_settings.h"))
if not matches:
    if libdeps_root.is_dir() and any(libdeps_root.glob("*")):
        print("::error::No Arduino-wolfSSL user_settings.h under .pio/libdeps")
        sys.exit(1)
    print("Skipping wolfSSL patch until libdeps are installed")
else:
    for settings in matches:
        patch_user_settings(settings)
