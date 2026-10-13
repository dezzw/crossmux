find_package(OpenSSL REQUIRED)

add_executable(OtaSignatureVerifyTest
  OtaSignatureVerifyTest.cpp
  ${REPO_ROOT}/lib/JsonParser/OtaSignatureVerify.cpp
)
target_compile_definitions(OtaSignatureVerifyTest PRIVATE CROSSPOINT_HOST_TEST)
target_include_directories(OtaSignatureVerifyTest PRIVATE ${REPO_ROOT}/lib/JsonParser ${REPO_ROOT})
target_link_libraries(OtaSignatureVerifyTest PRIVATE crosspoint_test_common GTest::gtest_main OpenSSL::SSL OpenSSL::Crypto)
gtest_discover_tests(OtaSignatureVerifyTest)
