#!/usr/bin/env python3
"""Seed crossmux-patches/SecureNet before the first PlatformIO lib symlink install."""

from pathlib import Path

from securenet_overlay import apply_patches


def main() -> None:
    project_dir = Path(__file__).resolve().parent.parent
    apply_patches(project_dir)


if __name__ == "__main__":
    main()
