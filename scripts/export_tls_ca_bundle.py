#!/usr/bin/env python3
"""Regenerate src/network/TlsCaBundle.cpp from Mozilla CA extract."""

import re
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
    'Sectigo Public Server Authentication Root E46',
    'Sectigo Public Server Authentication Root R46',
    'USERTrust RSA Certification Authority',
    'USERTrust ECC Certification Authority',
    'GlobalSign Root CA - R3',
    'GlobalSign Root CA',
    'GTS Root R1',
    'GTS Root R3',
    'GTS Root R4',
]


def main():
    cacert = Path('/tmp/cacert.pem')
    if not cacert.is_file():
        subprocess.check_call(['curl', '-fsSL', 'https://curl.se/ca/cacert.pem', '-o', str(cacert)])
    all_certs = re.findall(
        r'-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----',
        cacert.read_text(),
        flags=re.DOTALL,
    )
    selected = []
    seen = set()
    for cert in all_certs:
        cert = cert.strip() + '\n'
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
    if len(selected) != len(NEEDED):
        missing = [name for name in NEEDED if name not in seen]
        raise SystemExit(f'Missing CA roots: {missing}')
    PEM_OUT.write_text('\n\n'.join(selected) + '\n')
    literals = []
    for cert in selected:
        for line in cert.splitlines():
            escaped_line = line.replace('\\', '\\\\').replace('"', '\\"')
            literals.append(f'    "{escaped_line}\\n"')
    body = '\n'.join(literals)
    CPP_OUT.write_text(
        '#include "TlsCaBundle.h"\n\nnamespace TlsCaBundle {\n\n'
        f'static const char kPemRoots[] =\n{body};\n\n'
        'const char* pemRoots() { return kPemRoots; }\n\n}  // namespace TlsCaBundle\n'
    )
    print(f'Wrote {PEM_OUT} ({PEM_OUT.stat().st_size} bytes)')


if __name__ == '__main__':
    main()
