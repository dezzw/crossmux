import copy
import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import yaml

SCRIPTS = Path(__file__).resolve().parents[1]
ROOT = SCRIPTS.parent
sys.path.insert(0, str(SCRIPTS))

import build_nightly_index
import nightly_targets
import ota_test_fixtures
import ota_manifest
import package_nightly_target
import sign_ota_release
import verify_nightly_release

NIGHTLY_WORKFLOW_PATH = ROOT / '.github/workflows/nightly.yml'
STABLE_UPLOAD_STEP_NAME = 'Upload signed stable manifest to rolling stable release (last)'
STABLE_PUBLISH_JOB_IF = "needs.prepare.outputs.channel == 'stable'"
SOFT_FAIL_RUN_PATTERNS = ('|| true', '|| :', 'set +e')


def load_nightly_workflow():
    with NIGHTLY_WORKFLOW_PATH.open(encoding='utf-8') as stream:
        return yaml.safe_load(stream)


def step_run(step):
    run = step.get('run', '')
    return run if isinstance(run, str) else ''


def publish_stable_release_job(workflow):
    return workflow['jobs']['publish_stable_release']


def assert_stable_release_notes_gate(workflow):
    job = publish_stable_release_job(workflow)
    steps = job['steps']
    if job.get('continue-on-error'):
        raise AssertionError('publish_stable_release must not use continue-on-error')

    job_if = job.get('if')
    if job_if != STABLE_PUBLISH_JOB_IF:
        raise AssertionError(
            f'publish_stable_release job if must be {STABLE_PUBLISH_JOB_IF!r} (got {job_if!r})'
        )

    sign_steps = [
        index
        for index, step in enumerate(steps)
        if step.get('name') == 'Sign stable OTA manifests (semver firmware URL)'
        and 'sign_ota_release.py' in step_run(step)
        and '--release-notes notes/ota-notes.md' in step_run(step)
    ]
    verify_steps = [index for index, step in enumerate(steps) if '--verify-release-notes' in step_run(step)]
    upload_steps = [index for index, step in enumerate(steps) if step.get('name') == STABLE_UPLOAD_STEP_NAME]

    if len(sign_steps) != 1:
        raise AssertionError('expected one stable sign step with --release-notes notes/ota-notes.md')
    if len(verify_steps) != 1:
        raise AssertionError('expected one stable verify step with --verify-release-notes')
    if len(upload_steps) != 1:
        raise AssertionError('expected one stable rolling upload step')

    sign_idx, verify_idx, upload_idx = sign_steps[0], verify_steps[0], upload_steps[0]
    if not sign_idx < verify_idx < upload_idx:
        raise AssertionError(
            f'stable release notes gate order must be sign < verify < upload '
            f'(got sign={sign_idx}, verify={verify_idx}, upload={upload_idx})'
        )

    for label, step in (
        ('sign', steps[sign_idx]),
        ('verify', steps[verify_idx]),
        ('upload', steps[upload_idx]),
    ):
        if 'if' in step:
            raise AssertionError(f'{label} step must not use if (got {step["if"]!r})')
        if step.get('continue-on-error'):
            raise AssertionError(f'{label} step must not use continue-on-error')
        run = step_run(step)
        for pattern in SOFT_FAIL_RUN_PATTERNS:
            if pattern in run:
                raise AssertionError(f'{label} step run must not contain {pattern!r}')


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

    def test_release_matrix_is_stable_only(self):
        stable = package_nightly_target.matrix('stable')['include']
        self.assertEqual(len(stable), 1)
        self.assertEqual(
            stable[0],
            {
                'targetId': 'waveshare_epaper_397',
                'deviceSlug': 'waveshare-epaper-397',
                'environment': 'waveshare_epaper_397',
            },
        )

    def test_nightly_env_packages_as_rc_build_profile(self):
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
            output = root / 'dist/nightly/waveshare_epaper_397'

            def git_value(_root, *args):
                return 'a' * (7 if '--short=7' in args else 40)

            with (
                mock.patch.object(package_nightly_target, 'verify_partition_csv'),
                mock.patch.object(package_nightly_target, 'find_boot_app0', return_value=boot_app0),
                mock.patch.object(package_nightly_target, 'git_value', side_effect=git_value),
            ):
                package_nightly_target.package_target(root, 'waveshare_epaper_397', 'nightly', output)

            manifest = json.loads(
                (output / nightly_targets.manifest_name('waveshare_epaper_397', 'global')).read_text()
            )
            self.assertEqual(manifest['environment'], 'waveshare_epaper_397_nightly')
            self.assertEqual(manifest['supportedChannels'], ['stable'])

    def test_runtime_models_and_board_tags_are_explicit(self):
        targets = nightly_targets.TARGETS
        target = targets['waveshare_epaper_397']
        self.assertEqual(target['models'], ['waveshare_epaper_397'])
        self.assertEqual(target['boardTag'], 'waveshare_epaper_397')
        self.assertEqual(target['deviceSlug'], 'waveshare-epaper-397')
        self.assertEqual(target['environments']['nightly'], 'waveshare_epaper_397_nightly')
        self.assertEqual(target['environments']['stable'], 'waveshare_epaper_397')
        self.assertEqual(target['supportedChannels'], ['stable'])
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
        self.assertIn('group: firmware-stable-publish', workflow)
        self.assertNotIn('workflow_dispatch:', workflow)
        self.assertIn("if: needs.prepare.outputs.channel == 'stable'", workflow)
        self.assertNotIn("if: needs.prepare.outputs.channel == 'nightly'", workflow)
        self.assertNotIn("if: always() && needs.prepare.result == 'success'", workflow)
        self.assertNotIn('  cleanup_github:', workflow)
        self.assertNotIn('python3 scripts/nightly_retention.py', workflow)
        rolling = workflow.split('- name: Publish rolling channel release last', 1)[1].split(
            '  verify_publish:', 1
        )[0]
        self.assertIn('--latest=false', rolling)
        self.assertNotIn('if [ "$CHANNEL" != stable ]', rolling)
        self.assertIn('legacy_assets=', rolling)
        verify = workflow.split('  verify_publish:', 1)[1]
        self.assertIn('needs: [prepare, publish_github]', verify)
        self.assertEqual(verify.count('python3 scripts/verify_nightly_release.py'), 1)

    def test_signed_manifest_firmware_url_matches_sign_script(self):
        private_pem = ota_test_fixtures.read_test_private_pem()
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
            self.assertEqual(manifests[0]['supportedChannels'], ['stable'])

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


class StableReleaseWorkflowGateTest(unittest.TestCase):
    def test_publish_stable_release_notes_gate(self):
        assert_stable_release_notes_gate(load_nightly_workflow())

    def test_rejects_verify_after_upload(self):
        workflow = copy.deepcopy(load_nightly_workflow())
        steps = publish_stable_release_job(workflow)['steps']
        verify = next(step for step in steps if '--verify-release-notes' in step_run(step))
        steps.remove(verify)
        steps.append(verify)
        with self.assertRaises(AssertionError):
            assert_stable_release_notes_gate(workflow)

    def test_rejects_verify_run_or_true(self):
        workflow = copy.deepcopy(load_nightly_workflow())
        steps = publish_stable_release_job(workflow)['steps']
        verify = next(step for step in steps if '--verify-release-notes' in step_run(step))
        verify['run'] = step_run(verify) + '\n  true || true\n'
        with self.assertRaises(AssertionError):
            assert_stable_release_notes_gate(workflow)

    def test_rejects_verify_continue_on_error(self):
        workflow = copy.deepcopy(load_nightly_workflow())
        steps = publish_stable_release_job(workflow)['steps']
        verify = next(step for step in steps if '--verify-release-notes' in step_run(step))
        verify['continue-on-error'] = True
        with self.assertRaises(AssertionError):
            assert_stable_release_notes_gate(workflow)

    def test_rejects_missing_sign_release_notes_flag(self):
        workflow = copy.deepcopy(load_nightly_workflow())
        steps = publish_stable_release_job(workflow)['steps']
        sign = next(
            step
            for step in steps
            if step.get('name') == 'Sign stable OTA manifests (semver firmware URL)'
        )
        sign['run'] = step_run(sign).replace('--release-notes notes/ota-notes.md', '')
        with self.assertRaises(AssertionError):
            assert_stable_release_notes_gate(workflow)

    def test_rejects_verify_step_if_false(self):
        workflow = copy.deepcopy(load_nightly_workflow())
        steps = publish_stable_release_job(workflow)['steps']
        verify = next(step for step in steps if '--verify-release-notes' in step_run(step))
        verify['if'] = False
        with self.assertRaises(AssertionError):
            assert_stable_release_notes_gate(workflow)

    def test_rejects_sign_step_if(self):
        workflow = copy.deepcopy(load_nightly_workflow())
        steps = publish_stable_release_job(workflow)['steps']
        sign = next(
            step
            for step in steps
            if step.get('name') == 'Sign stable OTA manifests (semver firmware URL)'
        )
        sign['if'] = "success()"
        with self.assertRaises(AssertionError):
            assert_stable_release_notes_gate(workflow)

    def test_rejects_upload_step_if(self):
        workflow = copy.deepcopy(load_nightly_workflow())
        steps = publish_stable_release_job(workflow)['steps']
        upload = next(step for step in steps if step.get('name') == STABLE_UPLOAD_STEP_NAME)
        upload['if'] = "success()"
        with self.assertRaises(AssertionError):
            assert_stable_release_notes_gate(workflow)

    def test_rejects_altered_job_if(self):
        workflow = copy.deepcopy(load_nightly_workflow())
        publish_stable_release_job(workflow)['if'] = "always()"
        with self.assertRaises(AssertionError):
            assert_stable_release_notes_gate(workflow)


class StableIndexTest(unittest.TestCase):
    SAMPLE_RELEASE_NOTES = {
        'en': ['Stable note one', 'Stable note two'],
        'zh': ['稳定说明一', '稳定说明二'],
    }

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def write_pair(
        self, target_id, revision='a' * 40, sdk_revision='b' * 40, channel='stable'
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
                'version': '1.5.8',
                'crossmuxSha': revision,
                'sdkSha': sdk_revision,
                'assets': [{
                    'role': 'firmware',
                    'name': nightly_targets.asset_name(target_id, 'firmware.bin'),
                    'sha256': 'd' * 64,
                }],
            }
            (self.root / nightly_targets.manifest_name(target_id, flavor)).write_text(json.dumps(manifest))

    def write_all_pairs(self, revision='a' * 40, sdk_revision='b' * 40, channel='stable'):
        for target_id in nightly_targets.targets_for(channel):
            self.write_pair(target_id, revision, sdk_revision, channel)

    def test_builds_complete_index(self):
        self.write_all_pairs()
        index = build_nightly_index.build_index(
            self.root,
            'https://example.com/stable/',
            '2026-08-26T00:00:00Z',
            'stable-test',
            'stable',
            release_notes=self.SAMPLE_RELEASE_NOTES,
        )
        self.assertEqual(set(index['targets']), set(nightly_targets.TARGETS))
        self.assertEqual(
            index['targets']['waveshare_epaper_397']['variants']['zh-CN']['manifestUrl'],
            'https://example.com/stable/waveshare-epaper-397-cn-manifest.json',
        )

    def test_rejects_incomplete_target_set(self):
        self.write_all_pairs()
        (self.root / nightly_targets.manifest_name('waveshare_epaper_397', 'zh-CN')).unlink()
        with self.assertRaisesRegex(ValueError, 'expected one waveshare_epaper_397/zh-CN manifest'):
            build_nightly_index.build_index(
                self.root,
                'https://example.com/',
                'now',
                'test',
                'stable',
                release_notes=self.SAMPLE_RELEASE_NOTES,
            )

    def test_rejects_mismatched_sdk_pair(self):
        self.write_all_pairs()
        chinese = self.root / nightly_targets.manifest_name('waveshare_epaper_397', 'zh-CN')
        manifest = json.loads(chinese.read_text())
        manifest['sdkSha'] = 'c' * 40
        chinese.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, 'SDK revisions do not match'):
            build_nightly_index.build_index(
                self.root,
                'https://example.com/',
                'now',
                'test',
                'stable',
                release_notes=self.SAMPLE_RELEASE_NOTES,
            )

    def test_rejects_mismatched_asset_pair(self):
        self.write_all_pairs()
        chinese = self.root / nightly_targets.manifest_name('waveshare_epaper_397', 'zh-CN')
        manifest = json.loads(chinese.read_text())
        manifest['assets'][0]['sha256'] = 'e' * 64
        chinese.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, 'assets do not match'):
            build_nightly_index.build_index(
                self.root,
                'https://example.com/',
                'now',
                'test',
                'stable',
                release_notes=self.SAMPLE_RELEASE_NOTES,
            )

    def test_rejects_mixed_target_revisions(self):
        self.write_all_pairs()
        chinese = self.root / nightly_targets.manifest_name('waveshare_epaper_397', 'zh-CN')
        manifest = json.loads(chinese.read_text())
        manifest['crossmuxSha'] = 'c' * 40
        chinese.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, 'flavor revisions do not match'):
            build_nightly_index.build_index(
                self.root,
                'https://example.com/',
                'now',
                'test',
                'stable',
                release_notes=self.SAMPLE_RELEASE_NOTES,
            )


class ValidatePublishedUrlTest(unittest.TestCase):
    def test_accepts_urls_under_index_repository(self):
        index = 'https://github.com/dezzw/crossmux/releases/download/stable/release-index.json'
        manifest = (
            'https://github.com/dezzw/crossmux/releases/download/stable-build-deadbeef-1-1/'
            'waveshare-epaper-397-global-manifest.json'
        )
        verify_nightly_release.validate_url(manifest, index, 'stable')

    def test_rejects_manifest_from_different_owner_than_index(self):
        index = 'https://github.com/dezzw/crossmux/releases/download/stable/release-index.json'
        manifest = (
            'https://github.com/0x1abin/crossmux/releases/download/stable-build-deadbeef-1-1/'
            'waveshare-epaper-397-global-manifest.json'
        )
        with self.assertRaisesRegex(ValueError, 'outside the expected release path'):
            verify_nightly_release.validate_url(manifest, index, 'stable')

    def test_rejects_index_from_different_owner_than_manifest(self):
        index = 'https://github.com/0x1abin/crossmux/releases/download/stable/release-index.json'
        manifest = (
            'https://github.com/dezzw/crossmux/releases/download/stable-build-deadbeef-1-1/'
            'waveshare-epaper-397-global-manifest.json'
        )
        with self.assertRaisesRegex(ValueError, 'outside the expected release path'):
            verify_nightly_release.validate_url(manifest, index, 'stable')

    def test_rejects_non_https_index_url(self):
        index = 'http://github.com/dezzw/crossmux/releases/download/stable/release-index.json'
        with self.assertRaisesRegex(ValueError, 'index URL is not a GitHub releases download path'):
            verify_nightly_release.github_repo_prefix(index)


class PublishedStableTest(unittest.TestCase):
    github_owner = '0x1abin'
    github_repo = 'crossmux'
    RELEASE_NOTES = {
        'global': ['Stable note one', 'Stable note two'],
        'zh-CN': ['稳定说明一', '稳定说明二'],
    }

    def setUp(self):
        base = f'https://github.com/{self.github_owner}/{self.github_repo}/releases/download'
        self.index_url = f'{base}/stable/release-index.json'
        self.release_url = f'{base}/stable-build-test/'
        self.current_sha = 'a' * 40
        self.old_sha = 'c' * 40
        self.sdk_sha = 'b' * 40
        self.store = {}
        self.fetches = {}
        targets = {}
        for target_id, target in nightly_targets.TARGETS.items():
            revision = self.current_sha
            assets = []
            for role, name, offset in verify_nightly_release.expected_assets(target_id, 'stable'):
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
                version = '1.5.8'
                manifest = {
                    'schemaVersion': 1,
                    'channel': 'stable',
                    'targetId': target_id,
                    'models': target['models'],
                    'deviceSlug': target['deviceSlug'],
                    'boardTag': target['boardTag'],
                    'supportedChannels': target['supportedChannels'],
                    'environment': nightly_targets.environment_for(target_id, 'stable', flavor),
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
            'channel': 'stable',
            'updatedAt': 'now',
            'buildId': 'test',
            'releaseNotes': self.RELEASE_NOTES,
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
            self.index_url, self.current_sha, 'stable', self.fetch
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
            verify_nightly_release.verify_release(self.index_url, self.current_sha, 'stable', self.fetch)

    def test_rejects_missing_target(self):
        self.index['targets'].pop('waveshare_epaper_397')
        self.write_index()
        with self.assertRaisesRegex(ValueError, 'canonical target set'):
            verify_nightly_release.verify_release(self.index_url, self.current_sha, 'stable', self.fetch)

    def test_rejects_manifest_difference(self):
        url = self.release_url + nightly_targets.manifest_name('waveshare_epaper_397', 'zh-CN')
        manifest = json.loads(self.store[url])
        manifest['unexpected'] = True
        self.store[url] = json.dumps(manifest).encode()
        with self.assertRaisesRegex(ValueError, 'differ beyond flavor'):
            verify_nightly_release.verify_release(self.index_url, self.current_sha, 'stable', self.fetch)

    def test_rejects_corrupt_asset(self):
        url = self.release_url + nightly_targets.asset_name('waveshare_epaper_397', 'firmware.bin')
        self.store[url] += b'corrupt'
        with self.assertRaisesRegex(ValueError, 'size or SHA-256'):
            verify_nightly_release.verify_release(self.index_url, self.current_sha, 'stable', self.fetch)

    def test_rejects_manifest_owner_mismatch(self):
        foreign_owner = 'dezzw' if self.github_owner == '0x1abin' else '0x1abin'
        foreign = (
            f'https://github.com/{foreign_owner}/{self.github_repo}/releases/download/'
            'stable-build-test/'
        )
        target_id = 'waveshare_epaper_397'
        flavor = 'global'
        name = nightly_targets.manifest_name(target_id, flavor)
        self.store[foreign + name] = self.store[self.release_url + name]
        self.index['targets'][target_id]['variants'][flavor]['manifestUrl'] = foreign + name
        self.write_index()
        with self.assertRaisesRegex(ValueError, 'outside the expected release path'):
            verify_nightly_release.verify_release(self.index_url, self.current_sha, 'stable', self.fetch)


class PublishedStableDezzwTest(PublishedStableTest):
    github_owner = 'dezzw'
    github_repo = 'crossmux'


if __name__ == '__main__':
    unittest.main()
