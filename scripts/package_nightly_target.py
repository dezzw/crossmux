#!/usr/bin/env python3
"""Package one model-specific firmware target and emit compatibility manifests."""

import argparse
import configparser
import csv
import hashlib
import json
import os
import shutil
import subprocess
from pathlib import Path

from nightly_targets import (
    FLAVOR_TOKENS,
    CHANNELS,
    TARGETS,
    asset_name,
    environment_for,
    manifest_name,
    matrix,
    version_for,
)
FULL_INSTALL_SEGMENTS = (
    ('bootloader', 'bootloader.bin', 0x0000),
    ('partitions', 'partitions.bin', 0x8000),
    ('boot_app0', 'boot_app0.bin', 0xE000),
    ('firmware', 'firmware.bin', 0x10000),
)
STABLE_C3_SEGMENTS = (
    ('bootloader', 'bootloader.bin', 0x0000),
    ('partitions', 'partitions.bin', 0x8000),
    ('firmware', 'firmware.bin', 0x10000),
)
OTA_SEGMENT = (('firmware', 'firmware.bin', 0x10000),)
EXPECTED_PARTITIONS = {
    'otadata': (0xE000, 0x2000),
    'app0': (0x10000, 0x640000),
    'app1': (0x650000, 0x640000),
}


def sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def git_value(root, *args):
    return subprocess.check_output(['git', *args], cwd=root, text=True).strip()


def verify_partition_csv(root):
    found = {}
    with (root / 'partitions.csv').open(newline='') as stream:
        for row in csv.reader(line for line in stream if not line.lstrip().startswith('#')):
            if len(row) < 5:
                continue
            name = row[0].strip()
            if name in EXPECTED_PARTITIONS:
                found[name] = (int(row[3].strip(), 0), int(row[4].strip(), 0))
    if found != EXPECTED_PARTITIONS:
        raise SystemExit(f'partitions.csv has an unsupported OTA layout: {found!r}')


def find_boot_app0():
    path = Path.home() / '.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin'
    if not path.is_file():
        raise SystemExit(f'boot_app0.bin not found at {path}; build the environment first')
    return path


def copy_asset(source, destination):
    if not source.is_file():
        raise SystemExit(f'missing build asset: {source}')
    shutil.copyfile(source, destination)


def scan_firmware_markers(path, board_tag, version):
    board_marker = f'CROSSPOINT-BOARD-V1:{board_tag};'.encode()
    version_marker = f'CROSSPOINT-VERSION-V1:{version};'.encode()
    board_found = False
    version_found = False
    overlap = b''
    with path.open('rb') as stream:
        while chunk := stream.read(64 * 1024):
            data = overlap + chunk
            if not board_found and board_marker in data:
                board_found = True
            if not version_found and version_marker in data:
                version_found = True
            if board_found and version_found:
                return True, True
            keep = max(len(board_marker), len(version_marker)) - 1
            overlap = data[-keep:] if keep else b''
    return board_found, version_found


def verify_firmware(path, chip_id, board_tag, version):
    with path.open('rb') as stream:
        header = stream.read(24)
        if len(header) != 24 or header[0] != 0xE9:
            raise SystemExit(f'{path.name} is not an ESP image')
        actual_chip_id = int.from_bytes(header[12:14], 'little')
        if actual_chip_id != chip_id:
            raise SystemExit(
                f'{path.name} chip ID is 0x{actual_chip_id:04x}, expected 0x{chip_id:04x}'
            )

    board_found, version_found = scan_firmware_markers(path, board_tag, version)
    if not board_found:
        raise SystemExit(f'{path.name} does not contain board tag {board_tag!r}')
    if not version_found:
        raise SystemExit(f'{path.name} does not contain version tag {version!r}')


def package_target(root, target_id, channel, output):
    target = TARGETS[target_id]
    if channel not in target['supportedChannels']:
        raise SystemExit(f'{target_id} does not support the {channel} channel')
    environment = environment_for(target_id, channel, 'global')
    build = root / '.pio/build' / environment
    output.mkdir(parents=True, exist_ok=True)
    verify_partition_csv(root)

    segments = (
        FULL_INSTALL_SEGMENTS
        if target['fullInstall']
        else STABLE_C3_SEGMENTS
        if channel == 'stable'
        else OTA_SEGMENT
    )
    assets = []
    for role, source_name, offset in segments:
        source = find_boot_app0() if role == 'boot_app0' else build / source_name
        destination = output / asset_name(target_id, source_name)
        copy_asset(source, destination)
        assets.append({
            'role': role,
            'name': destination.name,
            'offset': offset,
            'size': destination.stat().st_size,
            'sha256': sha256(destination),
        })

    firmware = next(output / asset['name'] for asset in assets if asset['role'] == 'firmware')
    if firmware.stat().st_size > EXPECTED_PARTITIONS['app0'][1]:
        raise SystemExit(f'{firmware.name} exceeds the OTA app slot')

    config = configparser.ConfigParser()
    config.read(root / 'platformio.ini')
    rc_run = os.environ.get('CROSSPOINT_RC_RUN', '0').strip()
    manifest_version = version_for(
        config['crosspoint']['version'], target_id, channel, 'global', rc_run
    )
    verify_firmware(firmware, target['chipId'], target['boardTag'], manifest_version)

    manifest = {
        'schemaVersion': 1,
        'channel': channel,
        'targetId': target_id,
        'models': target['models'],
        'deviceSlug': target['deviceSlug'],
        'boardTag': target['boardTag'],
        'supportedChannels': target['supportedChannels'],
        'environment': environment,
        'chip': target['chip'],
        'version': manifest_version,
        'crossmuxSha': git_value(root, 'rev-parse', 'HEAD'),
        'sdkSha': git_value(root / 'freeink-sdk', 'rev-parse', 'HEAD'),
        'assets': assets,
    }
    if target['fullInstall']:
        manifest['partitionProfile'] = 'crossmux-sticky-v1'
        manifest['flash'] = {'size': 0x1000000, 'mode': 'dio', 'frequency': '80m'}

    manifest_paths = []
    for flavor in FLAVOR_TOKENS:
        manifest_path = output / manifest_name(target_id, flavor)
        manifest_path.write_text(json.dumps({**manifest, 'flavor': flavor}, indent=2) + '\n')
        manifest_paths.append(manifest_path)

    checksum_paths = [output / asset['name'] for asset in assets] + manifest_paths
    checksum_name = f"{target['deviceSlug']}-SHA256SUMS"
    (output / checksum_name).write_text(
        ''.join(f'{sha256(path)}  {path.name}\n' for path in checksum_paths)
    )
    print(f"Packaged {target_id} {manifest['version']} in {output}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('target', nargs='?', choices=TARGETS)
    parser.add_argument('--channel', choices=CHANNELS, required=True)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--matrix', action='store_true')
    args = parser.parse_args()
    if args.matrix:
        print(json.dumps(matrix(args.channel), separators=(',', ':')))
        return
    if not args.target:
        parser.error('target is required unless --matrix is used')

    root = Path(__file__).resolve().parent.parent
    output = (args.output or root / 'dist' / args.channel / args.target).resolve()
    package_target(root, args.target, args.channel, output)


if __name__ == '__main__':
    main()
