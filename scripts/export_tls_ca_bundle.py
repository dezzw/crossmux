#!/usr/bin/env python3
"""Regenerate src/network/TlsCaBundle.cpp from Mozilla CA extract + pinned roots."""

import re
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PINNED_DIR = ROOT / 'scripts/tls/pinned'
PEM_OUT = ROOT / 'src/network/TlsCaBundle.pem'
CPP_OUT = ROOT / 'src/network/TlsCaBundle.cpp'

# Exact CN match (openssl subject CN=); avoids substring hits like "GlobalSign Root CA - R6".
MOZILLA_SPECS = [
    {'label': 'ISRG Root X1', 'cn': 'ISRG Root X1'},
    {'label': 'DigiCert Global Root G2', 'cn': 'DigiCert Global Root G2'},
    {'label': 'Amazon Root CA 1', 'cn': 'Amazon Root CA 1'},
    {'label': 'Sectigo Public Server Authentication Root E46', 'cn': 'Sectigo Public Server Authentication Root E46'},
    {'label': 'Sectigo Public Server Authentication Root R46', 'cn': 'Sectigo Public Server Authentication Root R46'},
    {'label': 'USERTrust RSA Certification Authority', 'cn': 'USERTrust RSA Certification Authority'},
    {'label': 'USERTrust ECC Certification Authority', 'cn': 'USERTrust ECC Certification Authority'},
    {'label': 'GlobalSign Root CA - R3', 'ou': 'GlobalSign Root CA - R3'},
    {'label': 'GTS Root R1', 'cn': 'GTS Root R1'},
    {'label': 'GTS Root R3', 'cn': 'GTS Root R3'},
    {'label': 'GTS Root R4', 'cn': 'GTS Root R4'},
]

PINNED = [
    {
        'label': 'GlobalSign Root CA',
        'path': PINNED_DIR / 'GlobalSign_Root_CA.pem',
        'sha256': 'EB:D4:10:40:E4:BB:3E:C7:42:C9:E3:81:D3:1E:F2:A4:1A:48:B6:68:5C:96:E7:CE:F3:C1:DF:6C:D4:33:1C:99',
    },
]


def cert_field(subject_line: str, key: str) -> str:
    match = re.search(rf'{key}\s*=\s*([^,/]+)', subject_line)
    return match.group(1).strip() if match else ''


def cert_cn(subject_line: str) -> str:
    return cert_field(subject_line, 'CN')


def cert_ou(subject_line: str) -> str:
    return cert_field(subject_line, 'OU')


def cert_fingerprint_sha256(pem: str) -> str:
    with tempfile.NamedTemporaryFile('w', delete=False, suffix='.pem') as handle:
        handle.write(pem)
        path = handle.name
    fp = subprocess.check_output(['openssl', 'x509', '-in', path, '-noout', '-fingerprint', '-sha256'], text=True)
    Path(path).unlink(missing_ok=True)
    return fp.split('=', 1)[1].strip().upper()


def normalize_fp(fp: str) -> str:
    return fp.replace('SHA256 Fingerprint=', '').strip().upper()


def main():
    cacert = Path('/tmp/cacert.pem')
    if not cacert.is_file():
        subprocess.check_call(['curl', '-fsSL', 'https://curl.se/ca/cacert.pem', '-o', str(cacert)])
    all_certs = re.findall(
        r'-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----',
        cacert.read_text(),
        flags=re.DOTALL,
    )
    selected: list[str] = []
    for spec in MOZILLA_SPECS:
        found = None
        for cert in all_certs:
            cert = cert.strip() + '\n'
            with tempfile.NamedTemporaryFile('w', delete=False, suffix='.pem') as handle:
                handle.write(cert)
                path = handle.name
            subject = subprocess.check_output(['openssl', 'x509', '-in', path, '-noout', '-subject'], text=True)
            Path(path).unlink(missing_ok=True)
            if 'cn' in spec and cert_cn(subject) != spec['cn']:
                continue
            if 'ou' in spec and cert_ou(subject) != spec['ou']:
                continue
            found = cert.strip()
            break
        if not found:
            raise SystemExit(f"Missing CA root {spec['label']!r} in Mozilla bundle")
        selected.append(found)

    for spec in PINNED:
        pem_path = spec['path']
        if not pem_path.is_file():
            raise SystemExit(f'Missing pinned CA at {pem_path}')
        pem = pem_path.read_text().strip()
        fp = cert_fingerprint_sha256(pem + '\n')
        expected = normalize_fp(spec['sha256'])
        if fp != expected:
            raise SystemExit(f"Pinned {spec['label']} fingerprint mismatch: got {fp}, expected {expected}")
        selected.append(pem)

    order_labels = [s['label'] for s in MOZILLA_SPECS] + [s['label'] for s in PINNED]
    if len(selected) != len(order_labels):
        raise SystemExit('Internal error: CA selection count mismatch')

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
    print(f'Wrote {PEM_OUT} ({PEM_OUT.stat().st_size} bytes, {len(selected)} roots)')


if __name__ == '__main__':
    main()
