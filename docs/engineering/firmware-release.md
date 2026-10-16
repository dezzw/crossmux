# Firmware Release Architecture

CrossMux has two release channels, `stable` and `nightly`, managed by one
channel-aware pipeline. In the **dezzw/crossmux** fork the canonical hardware
target is **Waveshare ESP32-S3 ePaper 3.97**, published on both **Stable**
(tag `vX.Y.Z` on `main`) and **Nightly** channels from GitHub Releases only.

## Canonical targets

[`scripts/nightly_targets.py`](../../scripts/nightly_targets.py) is the release
source of truth despite its compatibility filename. Each target defines its
runtime models, artifact slug, embedded board tag, per-channel PlatformIO
environment, chip, install capability, and supported channels. The workflow,
packager, index builder, and tests import this table rather than copy it.

The sole entry is `waveshare_epaper_397`, built with `waveshare_epaper_397`
(Stable) and `waveshare_epaper_397_nightly` (Nightly). Each published image is
aliased by the compatibility `global` and `zh-CN` pointers.

## dezzw fork OTA signing (maintainers)

1. Run `python3 scripts/ota_keygen.py` locally. Commit the updated
   `src/network/OtaSigningPublicKey.h` and keep the generated private PEM offline.
2. Add the private PEM text to the repository secret **`OTA_SIGNING_KEY`**.
3. Set GitHub **Settings → Actions → General → Workflow permissions** to
   **Read and write**.
4. Tag the first stable release on `main` as `vX.Y.Z` matching `platformio.ini`
   `[crosspoint] version`.
5. Flash one device over USB with the firmware that embeds the matching public key
   before relying on OTA.

Devices fetch Stable manifests from
`https://github.com/dezzw/crossmux/releases/latest/download/waveshare-epaper-397-global-manifest.json`,
verify SHA-256 and an ECDSA P-256 signature, require a matching board tag, and
refuse downgrades. TLS uses the embedded CA bundle for GitHub and crossmux.com.

## Publishing

Each target job builds once and packages one binary set plus two compatibility
manifests. Packaging checks the ESP image chip ID, required board tag, partition
layout, app-slot size, and SHA-256 before emitting the manifests.

Publishing writes in this order:

1. immutable binaries and checksum files;
2. immutable target manifests;
3. the rolling GitHub index.

Every target selected for a channel must build successfully before publish runs.
Nightly also requires its previous rolling index because that index protects the
immediately preceding build during cleanup. After publish, CI resolves every
manifest and verifies each distinct asset's size and SHA-256. Cleanup then runs
for Nightly only.

The rolling index is the `release-index.json` asset of the `stable` or `nightly`
GitHub Release. Binaries and compatibility manifests live in an immutable
`<channel>-build-<sha>-<run>-<attempt>` GitHub Release.

## Index contract and failure behavior

The schema-v1 index contains `channel`, `updatedAt`, `buildId`, and a `targets`
map, plus optional regional `releaseNotes`. Each target repeats its identity and
channel capabilities and contains `global` and `zh-CN` pointers with version,
CrossMux SHA, SDK SHA, publish time, and immutable manifest URL.

Every target advances together only when both compatibility manifests are valid
for the same revision, SDK SHA, version string, and asset list. Mismatches fail
publication. See [`scripts/verify_nightly_release.py`](../../scripts/verify_nightly_release.py)
and [`scripts/tests/test_nightly_release.py`](../../scripts/tests/test_nightly_release.py).
