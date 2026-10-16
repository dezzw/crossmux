#include <gtest/gtest.h>

#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace {

std::string readFile(const char* path) {
  std::ifstream in(path);
  if (!in) return {};
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

int verifyIntermediateAgainstBundle(const std::string& rootsPem, const std::string& intermediatePem) {
  WOLFSSL_CERT_MANAGER* cm = wolfSSL_CertManagerNew();
  if (!cm) return MEMORY_E;
  const int loadCa =
      wolfSSL_CertManagerLoadCABuffer(cm, reinterpret_cast<const unsigned char*>(rootsPem.data()),
                                      static_cast<long>(rootsPem.size()), WOLFSSL_FILETYPE_PEM);
  if (loadCa != WOLFSSL_SUCCESS) {
    wolfSSL_CertManagerFree(cm);
    return loadCa;
  }
  const int verify =
      wolfSSL_CertManagerVerifyBuffer(cm, reinterpret_cast<const unsigned char*>(intermediatePem.data()),
                                      static_cast<long>(intermediatePem.size()), WOLFSSL_FILETYPE_PEM);
  wolfSSL_CertManagerFree(cm);
  return verify;
}

}  // namespace

class TlsCaChainVerifyTest : public ::testing::Test {
 protected:
  void SetUp() override { ASSERT_EQ(wolfSSL_Init(), WOLFSSL_SUCCESS); }
  void TearDown() override { wolfSSL_Cleanup(); }
};

TEST_F(TlsCaChainVerifyTest, GithubSectigoE36Intermediate) {
  const std::string roots = readFile(REPO_ROOT "/src/network/TlsCaBundle.pem");
  const std::string intermediate = readFile(REPO_ROOT "/test/fixtures/tls/github_sectigo_e36_intermediate.pem");
  ASSERT_FALSE(roots.empty());
  ASSERT_FALSE(intermediate.empty());
  const int rc = verifyIntermediateAgainstBundle(roots, intermediate);
#if TLS_WOLFSSL_SHA384_ENABLED
  EXPECT_EQ(rc, WOLFSSL_SUCCESS) << "wolfCrypt rc=" << rc;
#else
  EXPECT_NE(rc, WOLFSSL_SUCCESS) << "expected SHA-384 disabled verify failure, got rc=" << rc;
  EXPECT_TRUE(rc == ASN_SIG_HASH_E || rc == ASN_SIG_CONFIRM_E || rc == SIG_VERIFY_E)
      << "unexpected rc=" << rc;
#endif
}

TEST_F(TlsCaChainVerifyTest, CrossmuxGtsWe1Intermediate) {
  const std::string roots = readFile(REPO_ROOT "/src/network/TlsCaBundle.pem");
  const std::string intermediate = readFile(REPO_ROOT "/test/fixtures/tls/crossmux_gts_we1_intermediate.pem");
  ASSERT_FALSE(roots.empty());
  ASSERT_FALSE(intermediate.empty());
  const int rc = verifyIntermediateAgainstBundle(roots, intermediate);
#if TLS_WOLFSSL_SHA384_ENABLED
  EXPECT_EQ(rc, WOLFSSL_SUCCESS) << "wolfCrypt rc=" << rc;
#else
  EXPECT_NE(rc, WOLFSSL_SUCCESS) << "legacy wolfSSL config rc=" << rc;
#endif
}
