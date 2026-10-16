#!/usr/bin/env python3
"""Seed or verify crossmux-patches/SecureNet before PlatformIO resolves lib symlinks."""

import argparse
from pathlib import Path

from securenet_overlay import apply_patches, check_overlay


def main() -> None:
    parser = argparse.ArgumentParser(description="Seed or verify the patched SecureNet overlay.")
    parser.add_argument(
        "--check",
        action="store_true",
        help="Fail if the committed overlay does not match the freeink-sdk SecureNet sources and patches.",
    )
    args = parser.parse_args()
    project_dir = Path(__file__).resolve().parent.parent
    if args.check:
        check_overlay(project_dir)
        print("SecureNet overlay check passed.")
        return
    apply_patches(project_dir)


if __name__ == "__main__":
    main()
