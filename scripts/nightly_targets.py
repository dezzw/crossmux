"""Canonical firmware target mapping and artifact naming."""


TARGETS = {
    'waveshare_epaper_397': {
        'deviceSlug': 'waveshare-epaper-397',
        'models': ['waveshare_epaper_397'],
        'boardTag': 'waveshare_epaper_397',
        'chip': 'ESP32-S3',
        'chipId': 0x0009,
        'environments': {
            'nightly': 'waveshare_epaper_397_nightly',
            'stable': 'waveshare_epaper_397',
        },
        'supportedChannels': ['stable'],
        'fullInstall': True,
    },
}

FLAVOR_TOKENS = {'global': 'global', 'zh-CN': 'cn'}
# OTA / rolling-release channels (device and publish_github index).
CHANNELS = ('stable',)
# RC packaging for waveshare_epaper_397_nightly CI builds (not an OTA channel).
PACKAGING_CHANNELS = ('stable', 'nightly')


def targets_for(channel):
    if channel == 'nightly':
        return dict(TARGETS)
    if channel not in CHANNELS:
        raise KeyError(channel)
    return {target_id: target for target_id, target in TARGETS.items() if channel in target['supportedChannels']}


def environment_for(target_id, channel, flavor):
    if flavor not in FLAVOR_TOKENS:
        raise KeyError(flavor)
    return TARGETS[target_id]['environments'][channel]


def version_for(base_version, target_id, channel, flavor, rc_run):
    target = TARGETS[target_id]
    if channel == 'stable':
        return base_version
    if channel != 'nightly':
        raise KeyError(channel)
    parts = [base_version, target['deviceSlug']]
    if flavor not in FLAVOR_TOKENS:
        raise KeyError(flavor)
    run = str(rc_run).strip()
    if not run.isdigit():
        raise ValueError(f'nightly rc run must be numeric, got {rc_run!r}')
    return f"{'-'.join(parts)}-rc.{run}"


def asset_name(target_id, source_name):
    target = TARGETS[target_id]
    return f"{target['deviceSlug']}-{source_name}"


def manifest_name(target_id, flavor):
    target = TARGETS[target_id]
    return f"{target['deviceSlug']}-{FLAVOR_TOKENS[flavor]}-manifest.json"


def matrix(channel):
    return {
        'include': [
            {
                'targetId': target_id,
                'deviceSlug': target['deviceSlug'],
                'environment': environment_for(target_id, channel, 'global'),
            }
            for target_id, target in targets_for(channel).items()
        ]
    }
