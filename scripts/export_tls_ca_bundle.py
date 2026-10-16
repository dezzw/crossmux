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

MOZILLA_SPECS = [
    {'label': 'ISRG Root X1', 'cn': 'ISRG Root X1',
     'sha256': '96:BC:EC:06:26:49:76:F3:74:60:77:9A:CF:28:C5:A7:CF:E8:A3:C0:AA:E1:1A:8F:FC:EE:05:C0:BD:DF:08:C6'},
    {'label': 'DigiCert Global Root G2', 'cn': 'DigiCert Global Root G2',
     'sha256': 'CB:3C:CB:B7:60:31:E5:E0:13:8F:8D:D3:9A:23:F9:DE:47:FF:C3:5E:43:C1:14:4C:EA:27:D4:6A:5A:B1:CB:5F'},
    {'label': 'Amazon Root CA 1', 'cn': 'Amazon Root CA 1',
     'sha256': '8E:CD:E6:88:4F:3D:87:B1:12:5B:A3:1A:C3:FC:B1:3D:70:16:DE:7F:57:CC:90:4F:E1:CB:97:C6:AE:98:19:6E'},
    {'label': 'Sectigo Public Server Authentication Root E46', 'cn': 'Sectigo Public Server Authentication Root E46',
     'sha256': 'C9:0F:26:F0:FB:1B:40:18:B2:22:27:51:9B:5C:A2:B5:3E:2C:A5:B3:BE:5C:F1:8E:FE:1B:EF:47:38:0C:53:83'},
    {'label': 'Sectigo Public Server Authentication Root R46', 'cn': 'Sectigo Public Server Authentication Root R46',
     'sha256': '7B:B6:47:A6:2A:EE:AC:88:BF:25:7A:A5:22:D0:1F:FE:A3:95:E0:AB:45:C7:3F:93:F6:56:54:EC:38:F2:5A:06'},
    {'label': 'USERTrust RSA Certification Authority', 'cn': 'USERTrust RSA Certification Authority',
     'sha256': 'E7:93:C9:B0:2F:D8:AA:13:E2:1C:31:22:8A:CC:B0:81:19:64:3B:74:9C:89:89:64:B1:74:6D:46:C3:D4:CB:D2'},
    {'label': 'USERTrust ECC Certification Authority', 'cn': 'USERTrust ECC Certification Authority',
     'sha256': '4F:F4:60:D5:4B:9C:86:DA:BF:BC:FC:57:12:E0:40:0D:2B:ED:3F:BC:4D:4F:BD:AA:86:E0:6A:DC:D2:A9:AD:7A'},
    {'label': 'GlobalSign Root CA - R3', 'ou': 'GlobalSign Root CA - R3',
     'sha256': 'CB:B5:22:D7:B7:F1:27:AD:6A:01:13:86:5B:DF:1C:D4:10:2E:7D:07:59:AF:63:5A:7C:F4:72:0D:C9:63:C5:3B'},
    {'label': 'GTS Root R1', 'cn': 'GTS Root R1',
     'sha256': 'D9:47:43:2A:BD:E7:B7:FA:90:FC:2E:6B:59:10:1B:12:80:E0:E1:C7:E4:E4:0F:A3:C6:88:7F:FF:57:A7:F4:CF'},
    {'label': 'GTS Root R3', 'cn': 'GTS Root R3',
     'sha256': '34:D8:A7:3E:E2:08:D9:BC:DB:0D:95:65:20:93:4B:4E:40:E6:94:82:59:6E:8B:6F:73:C8:42:6B:01:0A:6F:48'},
    {'label': 'GTS Root R4', 'cn': 'GTS Root R4',
     'sha256': '34:9D:FA:40:58:C5:E2:63:12:3B:39:8A:E7:95:57:3C:4E:13:13:C8:3F:E6:8F:93:55:6C:D5:E8:03:1B:3C:7D'},
    {'label': 'TrustAsia TLS RSA Root CA', 'cn': 'TrustAsia TLS RSA Root CA',
     'sha256': '06:C0:8D:7D:AF:D8:76:97:1E:B1:12:4F:E6:7F:84:7E:C0:C7:A1:58:D3:EA:53:CB:E9:40:E2:EA:97:91:F4:C3'},
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
            fp = cert_fingerprint_sha256(cert)
            expected = normalize_fp(spec['sha256'])
            if fp != expected:
                raise SystemExit(f"Fingerprint mismatch for {spec['label']}: got {fp}, expected {expected}")
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
