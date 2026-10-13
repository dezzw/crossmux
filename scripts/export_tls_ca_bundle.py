#!/usr/bin/env python3
"""Regenerate src/network/TlsCaBundle.cpp from Mozilla CA extract."""

import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PEM_OUT = ROOT / 'src/network/TlsCaBundle.pem'
CPP_OUT = ROOT / 'src/network/TlsCaBundle.cpp'
NEEDED = [
    'ISRG Root X1',
    'DigiCert Global Root G2',
    'Amazon Root CA 1',
    'Sectigo Public Server Authentication Root R46',
    'USERTrust RSA Certification Authority',
]


def main():
    cacert = Path('/tmp/cacert.pem')
    if not cacert.is_file():
        subprocess.check_call(['curl', '-fsSL', 'https://curl.se/ca/cacert.pem', '-o', str(cacert)])
    blocks = cacert.read_text().split('-----BEGIN CERTIFICATE-----')
    selected = []
    seen = set()
    for block in blocks:
        if '-----END CERTIFICATE-----' not in block:
            continue
        cert = '-----BEGIN CERTIFICATE-----' + block
        with tempfile.NamedTemporaryFile('w', delete=False, suffix='.pem') as handle:
            handle.write(cert)
            path = handle.name
        subject = subprocess.check_output(['openssl', 'x509', '-in', path, '-noout', '-subject'], text=True)
        Path(path).unlink(missing_ok=True)
        for name in NEEDED:
            if name in subject and name not in seen:
                selected.append(cert.strip())
                seen.add(name)
                break
    PEM_OUT.write_text('\n\n'.join(selected) + '\n')
    pem = PEM_OUT.read_text()
    escaped = pem.replace('\\', '\\\\').replace('"', '\\"')
    chunks = [escaped[i : i + 72] for i in range(0, len(escaped), 72)]
    body = '\n'.join(f'    "{part}"' for part in chunks)
    CPP_OUT.write_text(
        '#include "TlsCaBundle.h"\n\nnamespace TlsCaBundle {\n\n'
        f'static const char kPemRoots[] =\n{body};\n\n'
        'const char* pemRoots() { return kPemRoots; }\n\n}  // namespace TlsCaBundle\n'
    )
    print(f'Wrote {PEM_OUT} ({PEM_OUT.stat().st_size} bytes)')


if __name__ == '__main__':
    main()
