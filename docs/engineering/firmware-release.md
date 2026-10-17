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
2. Add the private PEM text to the repository secret **`OTA_SIGNING_KEY`**
   (required for every stable and nightly publish job that signs OTA manifests).
3. Set GitHub **Settings → Actions → General → Workflow permissions** to
   **Read and write**.
4. Tag stable releases on `main` with a semver tag that **exactly equals**
   `[crosspoint] version` in `platformio.ini` (for example tag `1.6.0` when
   `version = 1.6.0`). The workflow rejects mismatched tag/version pairs.
5. Flash one device over USB with the firmware that embeds the matching public key
   before relying on OTA.

### One-time font + stable OTA bootstrap (Desmond)

After merging self-hosted fonts firmware, run these once on a **public**
`dezzw/crossmux` repository:

1. **Actions → Build & Publish SD Card Fonts → Run workflow.** This builds the core
   set, publishes immutable `fonts-m1-b4-r1` (revision increments on reruns), and
   updates the rolling `fonts-m1-b4` release (never marked GitHub “Latest”).
2. **Tag the first semver firmware release** on `main` (for example `1.6.0`) so
   **Firmware Release** publishes signed binaries and uploads
   `waveshare-epaper-397-global-manifest.json` to the rolling **`stable`** release.
3. Confirm anonymous HTTPS downloads work for both release tags (fonts and stable
   manifest) before shipping devices on this firmware. Stable publish verifies
   each signed global manifest’s `firmwareUrl` returns HTTP **200** before
   uploading the manifest to the rolling **`stable`** release (uploaded last,
   after the semver release assets exist).

Devices fetch Stable manifests from
`https://github.com/dezzw/crossmux/releases/download/stable/waveshare-epaper-397-global-manifest.json`
(the rolling `stable` GitHub Release is updated on each semver tag publish and is not marked Latest),
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
