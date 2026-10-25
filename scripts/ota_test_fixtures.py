#!/usr/bin/env python3
"""Locate or generate ephemeral OTA test signing fixtures for scripts and tests."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ENV_FIXTURE_DIR = 'CROSSPOINT_OTA_TEST_FIXTURE_DIR'
DEFAULT_OUTPUT_DIR = ROOT / 'build' / 'test' / 'ota-fixtures'

PRIVATE_NAME = 'ota_test_private.pem'
JSON_NAME = 'ota_signing_known_answer.json'


def fixture_dir() -> Path:
    raw = os.environ.get(ENV_FIXTURE_DIR, '').strip()
    return Path(raw) if raw else DEFAULT_OUTPUT_DIR


def private_key_path(base: Path | None = None) -> Path:
    return (base or fixture_dir()) / PRIVATE_NAME


def known_answer_json(base: Path | None = None) -> Path:
    return (base or fixture_dir()) / JSON_NAME


def ensure_generated(output_dir: Path | None = None) -> Path:
    """Generate fixtures if the private key is missing; return the fixture directory."""
    out = (output_dir or fixture_dir()).resolve()
    if not private_key_path(out).is_file():
        subprocess.check_call(
            [sys.executable, str(ROOT / 'scripts' / 'generate_ota_signing_fixture.py'), '--output-dir', str(out)]
        )
    return out


def read_test_private_pem(output_dir: Path | None = None) -> str:
    base = ensure_generated(output_dir)
    return private_key_path(base).read_text()
