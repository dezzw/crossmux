#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUNDLE="${ROOT}/src/network/TlsCaBundle.pem"
if [[ ! -f "$BUNDLE" ]]; then
  echo "::error::Missing TLS bundle at $BUNDLE"
  exit 1
fi

verify_host() {
  local host="$1"
  echo "TLS verify: ${host}"
  openssl s_client -connect "${host}:443" -servername "$host" -CAfile "$BUNDLE" -brief </dev/null 2>&1 | grep -q 'Verification: OK'
}

verify_host github.com
verify_host objects.githubusercontent.com
verify_host release-assets.githubusercontent.com
verify_host raw.githubusercontent.com
verify_host crossmux.com

echo "All TLS host checks passed."
