/* Host-test entry: mirrors CrossMux platformio.ini wolfSSL / TLS 1.3 flags + overrides. */
#ifndef CROSSMUX_WOLFSSL_DEVICE_USER_SETTINGS_H
#define CROSSMUX_WOLFSSL_DEVICE_USER_SETTINGS_H

#define WOLFSSL_TLS13
#define WOLFSSL_HAVE_ECC
#define WOLFSSL_HAVE_SP_ECC
#define WOLFSSL_HAVE_SP_RSA
#define WOLFSSL_SP_SMALL
#define WOLFSSL_SP_4096
#define HAVE_MAX_FRAGMENT
#define HAVE_FFDHE_2048
#define HAVE_CURVE25519
#define HAVE_SNI
#define HAVE_TLS_EXTENSIONS
#define HAVE_SUPPORTED_CURVES
#define HAVE_ECC
#define HAVE_AESGCM
#define HAVE_HKDF
#define HAVE_AEAD
#define WC_RSA_PSS
#define HAVE_RSA
#define HAVE_DH
#define HAVE_EXTENDED_MASTER
#define NO_PSK
#define NO_DSA
#define NO_RC4
#define NO_MD4
#define NO_MD5
#define NO_OLD_TLS

#include "wolfssl_crossmux_overrides.h"

#endif
