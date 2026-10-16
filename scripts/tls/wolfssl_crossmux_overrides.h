/* CrossMux device wolfSSL overrides (mirrors scripts/patch_wolfssl.py). */
#ifndef CROSSMUX_WOLFSSL_CROSSMUX_OVERRIDES_H
#define CROSSMUX_WOLFSSL_CROSSMUX_OVERRIDES_H

#if !defined(CROSSMUX_WOLFSSL_DISABLE_SHA384) || !CROSSMUX_WOLFSSL_DISABLE_SHA384
#ifndef WOLFSSL_SHA384
#define WOLFSSL_SHA384
#endif
#ifndef WOLFSSL_SHA512
#define WOLFSSL_SHA512
#endif
#ifndef WOLFSSL_SP_384
#define WOLFSSL_SP_384
#endif
#endif
#if !defined(CROSSMUX_WOLFSSL_ALT_CERT_CHAINS) || CROSSMUX_WOLFSSL_ALT_CERT_CHAINS
#ifndef WOLFSSL_ALT_CERT_CHAINS
#define WOLFSSL_ALT_CERT_CHAINS
#endif
#endif
#undef NO_DH
#ifndef HAVE_FFDHE_2048
#define HAVE_FFDHE_2048
#endif
#undef FP_MAX_BITS
#define FP_MAX_BITS 8192

#if !defined(CROSSMUX_WOLFSSL_DISABLE_SHA384) || !CROSSMUX_WOLFSSL_DISABLE_SHA384
#ifndef WOLFSSL_SHA384
#error "CrossMux wolfSSL overrides require WOLFSSL_SHA384"
#endif
#endif

#endif
