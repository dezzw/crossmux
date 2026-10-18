import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


SCRIPTS = Path(__file__).resolve().parents[1]
ROOT = SCRIPTS.parent
sys.path.insert(0, str(SCRIPTS))

import build_nightly_index
import nightly_retention
import nightly_targets
import ota_manifest
import package_nightly_target
import sign_ota_release
import verify_nightly_release


class NightlyTargetTest(unittest.TestCase):
    def test_fetch_retries_incomplete_reads(self):
        response = mock.MagicMock()
        response.__enter__.return_value.read.side_effect = [
            verify_nightly_release.http.client.IncompleteRead(b'partial', 1),
            b'complete',
        ]
        with mock.patch.object(
            verify_nightly_release.urllib.request, 'urlopen', return_value=response
        ) as urlopen, mock.patch.object(verify_nightly_release.time, 'sleep'):
            self.assertEqual(
                verify_nightly_release.fetch_bytes('https://github.com/example/asset.bin'),
                b'complete',
            )
        self.assertEqual(urlopen.call_count, 2)

    def test_matrix_has_single_waveshare_target_per_channel(self):
        nightly = package_nightly_target.matrix('nightly')['include']
        stable = package_nightly_target.matrix('stable')['include']
        self.assertEqual(len(nightly), 1)
        self.assertEqual(len(stable), 1)
        self.assertEqual(
            {entry['targetId'] for entry in nightly},
            set(nightly_targets.TARGETS),
        )
        self.assertEqual(
            nightly[0],
            {
                'targetId': 'waveshare_epaper_397',
                'deviceSlug': 'waveshare-epaper-397',
                'environment': 'waveshare_epaper_397_nightly',
            },
        )
        self.assertEqual(
            stable[0],
            {
                'targetId': 'waveshare_epaper_397',
                'deviceSlug': 'waveshare-epaper-397',
                'environment': 'waveshare_epaper_397',
            },
        )

    def test_runtime_models_and_board_tags_are_explicit(self):
        targets = nightly_targets.TARGETS
        target = targets['waveshare_epaper_397']
        self.assertEqual(target['models'], ['waveshare_epaper_397'])
        self.assertEqual(target['boardTag'], 'waveshare_epaper_397')
        self.assertEqual(target['deviceSlug'], 'waveshare-epaper-397')
        self.assertEqual(target['environments']['nightly'], 'waveshare_epaper_397_nightly')
        self.assertEqual(target['environments']['stable'], 'waveshare_epaper_397')
        self.assertEqual(target['supportedChannels'], ['nightly', 'stable'])
        self.assertTrue(target['fullInstall'])
        self.assertEqual(
            nightly_targets.environment_for('waveshare_epaper_397', 'nightly', 'global'),
            nightly_targets.environment_for('waveshare_epaper_397', 'nightly', 'zh-CN'),
        )

    def test_versions_are_nightly_release_candidates(self):
        self.assertEqual(
            nightly_targets.version_for(
                '1.5.7', 'waveshare_epaper_397', 'nightly', 'global', '12345'
            ),
            '1.5.7-waveshare-epaper-397-rc.12345',
        )
        self.assertEqual(
            nightly_targets.version_for(
                '1.5.7', 'waveshare_epaper_397', 'nightly', 'zh-CN', '12345'
            ),
            '1.5.7-waveshare-epaper-397-rc.12345',
        )
        self.assertNotIn(
            'beta',
            nightly_targets.version_for(
                '1.5.7', 'waveshare_epaper_397', 'nightly', 'global', '12345'
            ),
        )

    def test_workflow_packages_one_binary_set(self):
        workflow = (ROOT / '.github/workflows/nightly.yml').read_text()
        hardware_workflow = (ROOT / '.github/workflows/hardware-ci.yml').read_text()
        self.assertIn("find artifacts -type f -print", workflow)
        self.assertNotIn("find artifacts -path '*/global/*'", workflow)
        self.assertEqual(
            workflow.count('python3 scripts/package_nightly_target.py "${{ matrix.targetId }}"'), 1
        )
        self.assertIn('--channel "${{ needs.prepare.outputs.channel }}"', workflow)
        self.assertNotIn('for flavor in global cn', workflow)
        self.assertIn('pattern: firmware-stable-*', workflow)
        self.assertIn('merge-multiple: true', workflow)
        self.assertIn('mapfile -t firmware_assets', workflow)
        self.assertIn('Upload signed stable manifest to rolling stable release (last)', workflow)
        self.assertIn('python3 scripts/ota_manifest.py url', workflow)
        self.assertIn('Stable firmware releases are not configured', workflow)
        self.assertIn('gh release delete-asset "$CHANNEL" firmware-cn.bin', workflow)
        self.assertNotIn('--flavor', hardware_workflow)
        self.assertIn(
            '(cd "dist/nightly/waveshare_epaper_397" && sha256sum --check *-SHA256SUMS)',
            hardware_workflow,
        )

    def test_workflow_fails_closed_and_verifies_github_index(self):
        workflow = (ROOT / '.github/workflows/nightly.yml').read_text()
        self.assertIn("group: firmware-${{ github.event_name == 'push'", workflow)
        self.assertIn("if: needs.prepare.outputs.channel == 'stable'", workflow)
        self.assertGreaterEqual(workflow.count("if: needs.prepare.outputs.channel == 'nightly'"), 3)
        self.assertNotIn("if: always() && needs.prepare.result == 'success'", workflow)
        self.assertNotIn('--dir previous/global || true', workflow)
        self.assertNotIn('-o previous/cn.json || true', workflow)
        self.assertNotIn('--previous previous/', workflow)
        self.assertNotIn('name: nightly-previous-', workflow)
        self.assertNotIn('  publish_cn:', workflow)
        self.assertNotIn('  cleanup_cn:', workflow)
        self.assertNotIn('assets.crossmux.cn', workflow)
        rolling = workflow.split('- name: Publish rolling channel release last', 1)[1].split(
            '  verify_publish:', 1
        )[0]
        self.assertIn('--latest=false', rolling)
        self.assertIn('if [ "$CHANNEL" != stable ]', rolling)
        self.assertIn("*-global-manifest.json", rolling)
        self.assertIn('legacy_assets=', rolling)
        self.assertNotIn('xteink-firmware.bin', rolling)
        verify = workflow.split('  verify_publish:', 1)[1]
        self.assertIn('needs: [prepare, publish_github]', verify)
        self.assertEqual(verify.count('python3 scripts/verify_nightly_release.py'), 1)
        self.assertIn('  cleanup_github:\n    if:', workflow)
        self.assertEqual(workflow.count('python3 scripts/nightly_retention.py'), 1)
        self.assertIn('gh release delete "$build_tag"', workflow)
        self.assertIn('--cleanup-tag --yes', workflow)

    def test_signed_manifest_firmware_url_matches_sign_script(self):
        private_pem = (ROOT / 'test/fixtures/ota_test_private.pem').read_text()
        with tempfile.TemporaryDirectory() as temp:
            tmp = Path(temp)
            firmware = tmp / 'waveshare-epaper-397-firmware.bin'
            firmware.write_bytes(b'\x7fELF-test-firmware')
            manifest_in = tmp / 'manifest.json'
            manifest_in.write_text(
                json.dumps(
                    {
                        'version': '9.9.9-test',
                        'boardTag': 'waveshare_epaper_397',
                        'channel': 'stable',
                    }
                )
                + '\n'
            )
            signed = tmp / 'signed.json'
            download_url = 'https://github.com/example/releases/download/v9.9.9/waveshare-epaper-397-firmware.bin'
            with mock.patch.dict('os.environ', {'OTA_SIGNING_KEY': private_pem}):
                with mock.patch.object(
                    sys,
                    'argv',
                    [
                        'sign_ota_release.py',
                        '--firmware',
                        str(firmware),
                        '--manifest',
                        str(manifest_in),
                        '--download-url',
                        download_url,
                        '--output',
                        str(signed),
                    ],
                ):
                    sign_ota_release.main()
            url = ota_manifest.firmware_url_from_path(signed)
            self.assertEqual(url, download_url)
            with self.assertRaisesRegex(ValueError, 'empty'):
                ota_manifest.firmware_url_from_manifest({'firmware': {'url': ''}})

    def test_package_contains_one_binary_set_and_two_compatible_manifests(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            build = root / '.pio/build/waveshare_epaper_397_nightly'
            build.mkdir(parents=True)
            (build / 'bootloader.bin').write_bytes(b'bootloader')
            (build / 'partitions.bin').write_bytes(b'partitions')
            nightly_version = '1.6.0-waveshare-epaper-397-rc.0'
            (build / 'firmware.bin').write_bytes(
                self.write_image(board='waveshare_epaper_397', version=nightly_version).read_bytes()
            )
            boot_app0 = root / 'boot_app0.bin'
            boot_app0.write_bytes(b'boot_app0')
            (root / 'platformio.ini').write_text('[crosspoint]\nversion = 1.6.0\n')
            output = root / 'dist/waveshare_epaper_397'

            def git_value(_root, *args):
                return 'a' * (7 if '--short=7' in args else 40)

            with (
                mock.patch.object(package_nightly_target, 'verify_partition_csv'),
                mock.patch.object(package_nightly_target, 'find_boot_app0', return_value=boot_app0),
                mock.patch.object(package_nightly_target, 'git_value', side_effect=git_value),
            ):
                package_nightly_target.package_target(root, 'waveshare_epaper_397', 'nightly', output)

            self.assertEqual(
                {path.name for path in output.iterdir()},
                {
                    'waveshare-epaper-397-bootloader.bin',
                    'waveshare-epaper-397-partitions.bin',
                    'waveshare-epaper-397-boot_app0.bin',
                    'waveshare-epaper-397-firmware.bin',
                    'waveshare-epaper-397-global-manifest.json',
                    'waveshare-epaper-397-cn-manifest.json',
                    'waveshare-epaper-397-SHA256SUMS',
                },
            )
            manifests = [
                json.loads(
                    (output / nightly_targets.manifest_name('waveshare_epaper_397', flavor)).read_text()
                )
                for flavor in nightly_targets.FLAVOR_TOKENS
            ]
            self.assertEqual(manifests[0]['assets'], manifests[1]['assets'])
            self.assertEqual(
                {key: value for key, value in manifests[0].items() if key != 'flavor'},
                {key: value for key, value in manifests[1].items() if key != 'flavor'},
            )

            self.assertEqual(manifests[0]['channel'], 'nightly')
            self.assertEqual(manifests[0]['version'], '1.6.0-waveshare-epaper-397-rc.0')
            self.assertEqual(manifests[0]['supportedChannels'], ['nightly', 'stable'])

    def test_publish_github_job_runs_on_github_hosted_runner(self):
        workflow = (ROOT / '.github/workflows/nightly.yml').read_text()
        github_job = workflow.split('  publish_github:\n')[1].split('  verify_publish:\n')[0]
        self.assertIn('runs-on: ubuntu-latest', github_job)
        self.assertNotIn('COS_SECRET_', github_job)

    def write_image(self, chip_id=0x0009, board='waveshare_epaper_397', version=None):
        image = bytearray(24)
        image[0] = 0xE9
        image[12:14] = chip_id.to_bytes(2, 'little')
        image.extend(f'CROSSPOINT-BOARD-V1:{board};'.encode())
        if version is not None:
            image.extend(f'CROSSPOINT-VERSION-V1:{version};'.encode())
        temp = tempfile.NamedTemporaryFile(delete=False)
        temp.write(image)
        temp.close()
        self.addCleanup(Path(temp.name).unlink)
        return Path(temp.name)

    def test_waveshare_image_rejects_other_boards(self):
        version = '1.6.0-waveshare-epaper-397-rc.0'
        package_nightly_target.verify_firmware(
            self.write_image(board='waveshare_epaper_397', version=version),
            0x0009,
            'waveshare_epaper_397',
            version,
        )
        with self.assertRaises(SystemExit):
            package_nightly_target.verify_firmware(
                self.write_image(board='metalio_eink4', version=version),
                0x0009,
                'waveshare_epaper_397',
                version,
            )

    def test_rejects_wrong_chip_or_board(self):
        version = '1.6.0-waveshare-epaper-397-rc.0'
        package_nightly_target.verify_firmware(
            self.write_image(version=version), 0x0009, 'waveshare_epaper_397', version
        )
        with self.assertRaises(SystemExit):
            package_nightly_target.verify_firmware(
                self.write_image(chip_id=5, version=version), 0x0009, 'waveshare_epaper_397', version
            )
        with self.assertRaises(SystemExit):
            package_nightly_target.verify_firmware(
                self.write_image(board='murphy_m4', version=version),
                0x0009,
                'waveshare_epaper_397',
                version,
            )


class NightlyIndexTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def write_pair(
        self, target_id, revision='a' * 40, sdk_revision='b' * 40, channel='nightly'
    ):
        target = nightly_targets.TARGETS[target_id]
        for flavor in nightly_targets.FLAVOR_TOKENS:
            manifest = {
                'schemaVersion': 1,
                'channel': channel,
                'targetId': target_id,
                'models': target['models'],
                'deviceSlug': target['deviceSlug'],
                'boardTag': target['boardTag'],
                'supportedChannels': target['supportedChannels'],
                'environment': nightly_targets.environment_for(target_id, channel, flavor),
                'chip': target['chip'],
                'flavor': flavor,
                'version': '1.5.8' if channel == 'stable' else f'1.5.7-rc+{revision[:7]}',
                'crossmuxSha': revision,
                'sdkSha': sdk_revision,
                'assets': [{
                    'role': 'firmware',
                    'name': nightly_targets.asset_name(target_id, 'firmware.bin'),
                    'sha256': 'd' * 64,
                }],
            }
            (self.root / nightly_targets.manifest_name(target_id, flavor)).write_text(json.dumps(manifest))

    def write_all_pairs(self, revision='a' * 40, sdk_revision='b' * 40, channel='nightly'):
        for target_id in nightly_targets.targets_for(channel):
            self.write_pair(target_id, revision, sdk_revision, channel)

    def test_builds_complete_index(self):
        self.write_all_pairs()
        index = build_nightly_index.build_index(
            self.root,
            'https://example.com/nightly/',
            '2026-08-26T00:00:00Z',
            'nightly-test',
            'nightly',
        )
        self.assertEqual(set(index['targets']), set(nightly_targets.TARGETS))
        self.assertEqual(
            index['targets']['waveshare_epaper_397']['variants']['zh-CN']['manifestUrl'],
            'https://example.com/nightly/waveshare-epaper-397-cn-manifest.json',
        )

    def test_rejects_incomplete_target_set(self):
        self.write_all_pairs()
        (self.root / nightly_targets.manifest_name('waveshare_epaper_397', 'zh-CN')).unlink()
        with self.assertRaisesRegex(ValueError, 'expected one waveshare_epaper_397/zh-CN manifest'):
            build_nightly_index.build_index(
                self.root, 'https://example.com/', 'now', 'test', 'nightly'
            )

    def test_rejects_mismatched_sdk_pair(self):
        self.write_all_pairs()
        chinese = self.root / nightly_targets.manifest_name('waveshare_epaper_397', 'zh-CN')
        manifest = json.loads(chinese.read_text())
        manifest['sdkSha'] = 'c' * 40
        chinese.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, 'SDK revisions do not match'):
            build_nightly_index.build_index(
                self.root, 'https://example.com/', 'now', 'test', 'nightly'
            )

    def test_rejects_mismatched_asset_pair(self):
        self.write_all_pairs()
        chinese = self.root / nightly_targets.manifest_name('waveshare_epaper_397', 'zh-CN')
        manifest = json.loads(chinese.read_text())
        manifest['assets'][0]['sha256'] = 'e' * 64
        chinese.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, 'assets do not match'):
            build_nightly_index.build_index(
                self.root, 'https://example.com/', 'now', 'test', 'nightly'
            )

    def test_rejects_mixed_target_revisions(self):
        self.write_all_pairs()
        chinese = self.root / nightly_targets.manifest_name('waveshare_epaper_397', 'zh-CN')
        manifest = json.loads(chinese.read_text())
        manifest['crossmuxSha'] = 'c' * 40
        chinese.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, 'flavor revisions do not match'):
            build_nightly_index.build_index(
                self.root, 'https://example.com/', 'now', 'test', 'nightly'
            )


class NightlyRetentionTest(unittest.TestCase):
    github_owner = '0x1abin'
    github_repo = 'crossmux'

    def previous_index(self, first_build, second_build, owner=None, repo=None):
        owner = owner if owner is not None else self.github_owner
        repo = repo if repo is not None else self.github_repo
        targets = {}
        for index, target_id in enumerate(nightly_targets.TARGETS):
            build = second_build if index == 0 else first_build
            base = f'https://github.com/{owner}/{repo}/releases/download/{build}/'
            targets[target_id] = {
                'targetId': target_id,
                'variants': {
                    flavor: {
                        'manifestUrl': base + nightly_targets.manifest_name(target_id, flavor)
                    }
                    for flavor in nightly_targets.FLAVOR_TOKENS
                },
            }
        return {'schemaVersion': 1, 'channel': 'nightly', 'targets': targets}

    def test_github_keeps_current_and_every_build_in_previous_index(self):
        current = f'nightly-build-{"a" * 40}-10-1'
        previous = f'nightly-build-{"b" * 40}-9-1'
        previous_fallback = f'nightly-build-{"c" * 40}-8-1'
        obsolete = f'nightly-build-{"d" * 40}-7-1'
        candidates = '\n'.join((current, previous, previous_fallback, obsolete, 'v1.5.7'))
        self.assertEqual(
            nightly_retention.obsolete_builds(
                current,
                self.previous_index(previous, previous_fallback),
                candidates,
            ),
            sorted([previous, obsolete]),
        )

    def test_rejects_unexpected_previous_url_and_incomplete_listing(self):
        current = f'nightly-build-{"a" * 40}-10-1'
        previous = f'nightly-build-{"b" * 40}-9-1'
        index = self.previous_index(previous, previous)
        index['targets']['waveshare_epaper_397']['variants']['global']['manifestUrl'] = (
            'https://example.com/firmware.bin'
        )
        with self.assertRaisesRegex(ValueError, 'unexpected'):
            nightly_retention.obsolete_builds(current, index, current)
        with self.assertRaisesRegex(ValueError, 'missing from the candidate list'):
            nightly_retention.obsolete_builds(
                current, self.previous_index(previous, previous), previous
            )

    def test_dezzw_fork_manifest_urls_are_accepted(self):
        current = f'nightly-build-{"a" * 40}-10-1'
        previous = f'nightly-build-{"b" * 40}-9-1'
        previous_fallback = f'nightly-build-{"c" * 40}-8-1'
        obsolete = f'nightly-build-{"d" * 40}-7-1'
        candidates = '\n'.join((current, previous, previous_fallback, obsolete))
        self.assertEqual(
            nightly_retention.obsolete_builds(
                current,
                self.previous_index(previous, previous_fallback, owner='dezzw', repo='crossmux'),
                candidates,
            ),
            sorted([previous, obsolete]),
        )

    def test_rejects_non_https_manifest_url(self):
        current = f'nightly-build-{"a" * 40}-10-1'
        previous = f'nightly-build-{"b" * 40}-9-1'
        index = self.previous_index(previous, previous)
        manifest_url = index['targets']['waveshare_epaper_397']['variants']['global']['manifestUrl']
        index['targets']['waveshare_epaper_397']['variants']['global']['manifestUrl'] = manifest_url.replace(
            'https://', 'http://', 1
        )
        with self.assertRaisesRegex(ValueError, 'unexpected manifest URL path'):
            nightly_retention.obsolete_builds(current, index, current)

    def test_rejects_manifest_owner_mismatch(self):
        current = f'nightly-build-{"a" * 40}-10-1'
        previous = f'nightly-build-{"b" * 40}-9-1'
        index = self.previous_index(previous, previous, owner='dezzw', repo='crossmux')
        foreign_url = self.previous_index(previous, previous)['targets']['waveshare_epaper_397'][
            'variants'
        ]['global']['manifestUrl']
        index['targets']['waveshare_epaper_397']['variants']['zh-CN']['manifestUrl'] = foreign_url
        with self.assertRaisesRegex(ValueError, 'unexpected previous waveshare_epaper_397/zh-CN'):
            nightly_retention.obsolete_builds(current, index, current)


class ValidatePublishedUrlTest(unittest.TestCase):
    def test_accepts_urls_under_index_repository(self):
        index = 'https://github.com/dezzw/crossmux/releases/download/nightly/release-index.json'
        manifest = (
            'https://github.com/dezzw/crossmux/releases/download/nightly-build-deadbeef-1-1/'
            'waveshare-epaper-397-global-manifest.json'
        )
        verify_nightly_release.validate_url(manifest, index, 'nightly')

    def test_rejects_manifest_from_different_owner_than_index(self):
        index = 'https://github.com/dezzw/crossmux/releases/download/nightly/release-index.json'
        manifest = (
            'https://github.com/0x1abin/crossmux/releases/download/nightly-build-deadbeef-1-1/'
            'waveshare-epaper-397-global-manifest.json'
        )
        with self.assertRaisesRegex(ValueError, 'outside the expected release path'):
            verify_nightly_release.validate_url(manifest, index, 'nightly')

    def test_rejects_index_from_different_owner_than_manifest(self):
        index = 'https://github.com/0x1abin/crossmux/releases/download/nightly/release-index.json'
        manifest = (
            'https://github.com/dezzw/crossmux/releases/download/nightly-build-deadbeef-1-1/'
            'waveshare-epaper-397-global-manifest.json'
        )
        with self.assertRaisesRegex(ValueError, 'outside the expected release path'):
            verify_nightly_release.validate_url(manifest, index, 'nightly')

    def test_rejects_non_https_index_url(self):
        index = 'http://github.com/dezzw/crossmux/releases/download/nightly/release-index.json'
        with self.assertRaisesRegex(ValueError, 'index URL is not a GitHub releases download path'):
            verify_nightly_release.github_repo_prefix(index)


class PublishedNightlyTest(unittest.TestCase):
    github_owner = '0x1abin'
    github_repo = 'crossmux'

    def setUp(self):
        base = f'https://github.com/{self.github_owner}/{self.github_repo}/releases/download'
        self.index_url = f'{base}/nightly/release-index.json'
        self.release_url = f'{base}/nightly-build-test/'
        self.current_sha = 'a' * 40
        self.old_sha = 'c' * 40
        self.sdk_sha = 'b' * 40
        self.store = {}
        self.fetches = {}
        targets = {}
        for target_id, target in nightly_targets.TARGETS.items():
            revision = self.current_sha
            assets = []
            for role, name, offset in verify_nightly_release.expected_assets(target_id, 'nightly'):
                data = f'{target_id}/{role}'.encode()
                self.store[self.release_url + name] = data
                assets.append({
                    'role': role,
                    'name': name,
                    'offset': offset,
                    'size': len(data),
                    'sha256': hashlib.sha256(data).hexdigest(),
                })
            variants = {}
            for flavor in nightly_targets.FLAVOR_TOKENS:
                version = f'1.5.7-rc+{revision[:7]}'
                manifest = {
                    'schemaVersion': 1,
                    'channel': 'nightly',
                    'targetId': target_id,
                    'models': target['models'],
                    'deviceSlug': target['deviceSlug'],
                    'boardTag': target['boardTag'],
                    'supportedChannels': target['supportedChannels'],
                    'environment': nightly_targets.environment_for(target_id, 'nightly', flavor),
                    'flavor': flavor,
                    'version': version,
                    'crossmuxSha': revision,
                    'sdkSha': self.sdk_sha,
                    'assets': assets,
                }
                url = self.release_url + nightly_targets.manifest_name(target_id, flavor)
                self.store[url] = json.dumps(manifest).encode()
                variants[flavor] = {
                    'version': version,
                    'crossmuxSha': revision,
                    'sdkSha': self.sdk_sha,
                    'publishedAt': 'now',
                    'manifestUrl': url,
                }
            targets[target_id] = {
                'targetId': target_id,
                'models': target['models'],
                'deviceSlug': target['deviceSlug'],
                'boardTag': target['boardTag'],
                'supportedChannels': target['supportedChannels'],
                'variants': variants,
            }
        self.index = {
            'schemaVersion': 1,
            'channel': 'nightly',
            'updatedAt': 'now',
            'buildId': 'test',
            'targets': targets,
        }
        self.write_index()

    def write_index(self):
        self.store[self.index_url] = json.dumps(self.index).encode()

    def fetch(self, url):
        self.fetches[url] = self.fetches.get(url, 0) + 1
        return self.store[url]

    def test_verifies_complete_current_release_with_one_asset_fetch(self):
        result = verify_nightly_release.verify_release(
            self.index_url, self.current_sha, 'nightly', self.fetch
        )
        self.assertEqual(result['targets'], len(nightly_targets.TARGETS))
        self.assertEqual(result['currentTargets'], len(nightly_targets.TARGETS))
        for url in self.store:
            if url.endswith('.bin'):
                self.assertEqual(self.fetches[url], 1)

    def test_rejects_target_from_previous_revision(self):
        target_id = 'waveshare_epaper_397'
        for flavor in nightly_targets.FLAVOR_TOKENS:
            url = self.release_url + nightly_targets.manifest_name(target_id, flavor)
            manifest = json.loads(self.store[url])
            manifest['crossmuxSha'] = self.old_sha
            self.store[url] = json.dumps(manifest).encode()
            self.index['targets'][target_id]['variants'][flavor]['crossmuxSha'] = self.old_sha
        self.write_index()
        with self.assertRaisesRegex(ValueError, 'does not point to the current revision'):
            verify_nightly_release.verify_release(self.index_url, self.current_sha, 'nightly', self.fetch)

    def test_rejects_missing_target(self):
        self.index['targets'].pop('waveshare_epaper_397')
        self.write_index()
        with self.assertRaisesRegex(ValueError, 'canonical target set'):
            verify_nightly_release.verify_release(self.index_url, self.current_sha, 'nightly', self.fetch)

    def test_rejects_manifest_difference(self):
        url = self.release_url + nightly_targets.manifest_name('waveshare_epaper_397', 'zh-CN')
        manifest = json.loads(self.store[url])
        manifest['unexpected'] = True
        self.store[url] = json.dumps(manifest).encode()
        with self.assertRaisesRegex(ValueError, 'differ beyond flavor'):
            verify_nightly_release.verify_release(self.index_url, self.current_sha, 'nightly', self.fetch)

    def test_rejects_corrupt_asset(self):
        url = self.release_url + nightly_targets.asset_name('waveshare_epaper_397', 'firmware.bin')
        self.store[url] += b'corrupt'
        with self.assertRaisesRegex(ValueError, 'size or SHA-256'):
            verify_nightly_release.verify_release(self.index_url, self.current_sha, 'nightly', self.fetch)

    def test_rejects_manifest_owner_mismatch(self):
        foreign_owner = 'dezzw' if self.github_owner == '0x1abin' else '0x1abin'
        foreign = (
            f'https://github.com/{foreign_owner}/{self.github_repo}/releases/download/'
            'nightly-build-test/'
        )
        target_id = 'waveshare_epaper_397'
        flavor = 'global'
        name = nightly_targets.manifest_name(target_id, flavor)
        self.store[foreign + name] = self.store[self.release_url + name]
        self.index['targets'][target_id]['variants'][flavor]['manifestUrl'] = foreign + name
        self.write_index()
        with self.assertRaisesRegex(ValueError, 'outside the expected release path'):
            verify_nightly_release.verify_release(self.index_url, self.current_sha, 'nightly', self.fetch)


class PublishedNightlyDezzwTest(PublishedNightlyTest):
    github_owner = 'dezzw'
    github_repo = 'crossmux'


if __name__ == '__main__':
    unittest.main()
