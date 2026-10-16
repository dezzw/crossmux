import sys
from pathlib import Path

Import("env")

from securenet_overlay import apply_patches

PROJECT_DIR = Path(env.subst("$PROJECT_DIR"))

build_flags = env.get("BUILD_FLAGS", [])
uses_wolf = any("FREEINK_NET_WOLFSSL" in str(flag) for flag in build_flags)

if not uses_wolf:
    print("Skipping SecureClient patch (FREEINK_NET_WOLFSSL not enabled)")
else:
    source_dir = PROJECT_DIR / "freeink-sdk/libs/network/SecureNet"
    if not source_dir.is_dir():
        print(f"::error::SecureNet missing at {source_dir} with FREEINK_NET_WOLFSSL=1")
        sys.exit(1)
    apply_patches(PROJECT_DIR)
    print(f"Patched SecureClient overlay under {PROJECT_DIR / 'crossmux-patches/SecureNet'}")
