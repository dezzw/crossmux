#!/usr/bin/env python3
"""Plan deletion of Nightly builds not used by the current or previous index."""

import argparse
import json
import re
from pathlib import Path
from urllib.parse import urlparse

from nightly_targets import FLAVOR_TOKENS, TARGETS


BUILD_ID = r'[0-9a-f]{40}-[0-9]+-[0-9]+'
NIGHTLY_BUILD = rf'nightly-build-{BUILD_ID}'
NAMESPACE = re.compile(rf'^{NIGHTLY_BUILD}$')
FIND_NAMESPACE = re.compile(rf'(?<![a-z0-9-])({NIGHTLY_BUILD})(?![a-z0-9-])')
MANIFEST_PATH = re.compile(
    rf'^/0x1abin/crossmux/releases/download/({NIGHTLY_BUILD})/[^/]+-manifest\.json$'
)
HOST = 'github.com'


def referenced_builds(index):
    if not isinstance(index, dict) or index.get('schemaVersion') != 1 or index.get('channel') != 'nightly':
        raise ValueError('invalid previous Nightly index envelope')
    targets = index.get('targets')
    if not isinstance(targets, dict) or set(targets) != set(TARGETS):
        raise ValueError('previous Nightly index does not contain the canonical target set')

    builds = set()
    for target_id, entry in targets.items():
        variants = entry.get('variants') if isinstance(entry, dict) else None
        if not isinstance(entry, dict) or entry.get('targetId') != target_id or not isinstance(
            variants, dict
        ) or set(variants) != set(FLAVOR_TOKENS):
            raise ValueError(f'invalid previous {target_id} variant set')
        for flavor, pointer in variants.items():
            manifest_url = pointer.get('manifestUrl') if isinstance(pointer, dict) else None
            if not isinstance(manifest_url, str):
                raise ValueError(f'invalid previous {target_id}/{flavor} manifest URL')
            parsed = urlparse(manifest_url)
            match = MANIFEST_PATH.match(parsed.path)
            if parsed.scheme != 'https' or parsed.hostname != HOST or parsed.query or not match:
                raise ValueError(f'unexpected previous {target_id}/{flavor} manifest URL')
            builds.add(match.group(1))
    return builds


def obsolete_builds(current, previous_index, candidates):
    if not NAMESPACE.fullmatch(current):
        raise ValueError('invalid current GitHub build name')
    found = set(FIND_NAMESPACE.findall(candidates))
    if current not in found:
        raise ValueError('current GitHub build is missing from the candidate list')
    keep = referenced_builds(previous_index) | {current}
    return sorted(found - keep)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--current', required=True)
    parser.add_argument('--previous-index', type=Path, required=True)
    parser.add_argument('--candidates', type=Path, required=True)
    args = parser.parse_args()
    previous_index = json.loads(args.previous_index.read_text())
    candidates = args.candidates.read_text()
    for build in obsolete_builds(args.current, previous_index, candidates):
        print(build)


if __name__ == '__main__':
    main()
