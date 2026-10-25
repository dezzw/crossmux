# Host test fixtures

TLS certificate chains under `tls/` are checked in for offline TLS tests.

OTA signing test material is **not** stored here. Host tests and CI generate an
ephemeral ECDSA P-256 keypair at configure/build time:

```bash
python3 scripts/generate_ota_signing_fixture.py
```

By default, output goes to `build/test/ota-fixtures/` (gitignored). CMake runs
the same script when configuring `test/`; `pio run -t unit-tests` uses that
build directory.

The former committed test private key (`ota_test_private.pem`) was removed from
the repository. That key was only ever used for host tests and is **public and
void** — do not use it for release signing.
