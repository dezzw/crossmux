# Firmware Release Architecture

CrossMux has two release channels, `stable` and `nightly`, managed by one
channel-aware pipeline. In this fork the canonical hardware target is
**Waveshare ESP32-S3 ePaper 3.97**, published on the **Nightly** channel only.

## Canonical targets

[`scripts/nightly_targets.py`](../../scripts/nightly_targets.py) is the release
source of truth despite its compatibility filename. Each target defines its
runtime models, artifact slug, embedded board tag, per-channel PlatformIO
environment, chip, install capability, and supported channels. The workflow,
packager, index builder, and tests import this table rather than copy it.

The sole entry is `waveshare_epaper_397`, built with
`waveshare_epaper_397_nightly`. Each published image is aliased by the
compatibility `global` and `zh-CN` pointers.

Version-tag **Stable** releases are not configured until a stable PlatformIO
profile and target entry exist; the workflow fails closed when
`targets_for('stable')` is empty.

## Publishing

Each target job builds once and packages one binary set plus two compatibility
manifests. Packaging checks the ESP image chip ID, required board tag, partition
layout, app-slot size, and SHA-256 before emitting the manifests.

The global and China publish jobs run independently. Each writes in this order:

1. immutable binaries and checksum files;
2. immutable target manifests;
3. rolling regional indexes.

Every target selected for a channel must build successfully before either region
publishes. Nightly also requires its previous rolling index because that index
protects the immediately preceding build during cleanup. Once both regions
publish, CI resolves every manifest and verifies each distinct asset's size and
SHA-256. Cleanup then runs for Nightly only.

The global index is the `release-index.json` asset of the rolling `stable` or
`nightly` GitHub Release. Binaries and compatibility manifests live in an
immutable `<channel>-build-<sha>-<run>-<attempt>` GitHub Release. China indexes
are `/firmware/releases/<channel>/index.json`; target assets live under
`/firmware/builds/<channel>-build-<sha>-<run>-<attempt>/<target>/` in COS.

COS publishing runs only on the H2O self-hosted runner. It uses a version-pinned,
SHA-256-verified COSCLI binary from the runner's temporary directory.

## Index contract and failure behavior

The schema-v1 index contains `channel`, `updatedAt`, `buildId`, and a `targets`
map, plus optional regional `releaseNotes`. Each target repeats its identity and
channel capabilities and contains `global` and `zh-CN` pointers with version,
CrossMux SHA, SDK SHA, publish time, and immutable manifest URL.

Every target advances together only when both compatibility manifests are valid
for the same revision, SDK SHA, version string, and asset list. Mismatches fail
publication. See [`scripts/verify_nightly_release.py`](../../scripts/verify_nightly_release.py)
and [`scripts/tests/test_nightly_release.py`](../../scripts/tests/test_nightly_release.py).
