"""Inject CROSSPOINT_VERSION for waveshare_epaper_397_nightly (rc.<CI run number>)."""

import configparser
import os
import sys


def warn(msg):
    print(f'WARNING [nightly_crosspoint_version.py]: {msg}', file=sys.stderr)


def get_base_version(project_dir):
    ini_path = os.path.join(project_dir, 'platformio.ini')
    if not os.path.isfile(ini_path):
        warn(f'platformio.ini not found at {ini_path}; base version will be "0.0.0"')
        return '0.0.0'
    config = configparser.ConfigParser()
    config.read(ini_path, encoding='utf-8')
    if not config.has_option('crosspoint', 'version'):
        warn('No [crosspoint] version in platformio.ini; base version will be "0.0.0"')
        return '0.0.0'
    return config.get('crosspoint', 'version')


def inject_version(env):
    if env['PIOENV'] != 'waveshare_epaper_397_nightly':
        return
    run = os.environ.get('CROSSPOINT_RC_RUN', '0').strip()
    if not run.isdigit():
        warn(f'CROSSPOINT_RC_RUN={run!r} is not numeric; using 0')
        run = '0'
    base_version = get_base_version(env['PROJECT_DIR'])
    version_string = f'{base_version}-waveshare-epaper-397-rc.{run}'
    env.Append(CPPDEFINES=[('CROSSPOINT_VERSION', f'\\"{version_string}\\"')])
    print(f'CrossPoint nightly build version: {version_string}')


try:
    Import('env')  # noqa: F821  # type: ignore[name-defined]
    inject_version(env)  # noqa: F821  # type: ignore[name-defined]
except NameError:
    pass
