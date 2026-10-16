#pragma once

// Production OTA public key (PEM). Regenerate with: python3 scripts/ota_keygen.py
// Then add dist/ota-keys/ota_signing_private.pem to GitHub secret OTA_SIGNING_KEY.
inline constexpr char OTA_SIGNING_PUBLIC_KEY_PEM[] =
    "-----BEGIN PUBLIC KEY-----\n"
    "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE3sTIVenpqa8wxtokhH/oAaolqiPn\n"
    "olJL7qfpQPgV/ODVVt9v5NXT5kS7aeAezyylYlhjJNBWubm3vnOT2a+yYQ==\n"
    "-----END PUBLIC KEY-----\n";
