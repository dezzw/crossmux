#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUNDLE="${ROOT}/src/network/TlsCaBundle.pem"
VERIFY_BIN="${1:-${ROOT}/build/test/tls_chain_verify/TlsHostVerify}"

if [[ ! -f "$BUNDLE" ]]; then
  echo "::error::Missing TLS bundle at $BUNDLE"
  exit 1
fi

if [[ ! -x "$VERIFY_BIN" ]]; then
  echo "::error::Missing wolfSSL host verifier at $VERIFY_BIN (build TlsHostVerify first)"
  exit 1
fi

verify_host() {
  local host="$1"
  echo "TLS verify (wolfSSL): ${host}"
  "$VERIFY_BIN" "$host" "$BUNDLE"
}

verify_host github.com
verify_host objects.githubusercontent.com
verify_host release-assets.githubusercontent.com
verify_host raw.githubusercontent.com

echo "All TLS host checks passed."
