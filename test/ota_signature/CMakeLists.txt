include(${CMAKE_CURRENT_LIST_DIR}/../cmake/FindMbedTLS.cmake)
find_package(OpenSSL REQUIRED)

add_executable(OtaSignatureVerifyTest
  OtaSignatureVerifyTest.cpp
  ${REPO_ROOT}/lib/JsonParser/OtaSignatureVerify.cpp
)
target_compile_definitions(OtaSignatureVerifyTest PRIVATE CROSSPOINT_HOST_TEST)
target_include_directories(OtaSignatureVerifyTest PRIVATE ${REPO_ROOT}/lib/JsonParser ${REPO_ROOT})
target_link_libraries(OtaSignatureVerifyTest PRIVATE crosspoint_test_common GTest::gtest_main MbedTLS::mbedcrypto OpenSSL::Crypto OpenSSL::SSL)
gtest_discover_tests(OtaSignatureVerifyTest)

add_executable(OtaSignatureKnownAnswerTest
  OtaSignatureKnownAnswerTest.cpp
  ${REPO_ROOT}/lib/JsonParser/OtaSignatureVerify.cpp
  ${REPO_ROOT}/lib/JsonParser/OtaSigningPayload.cpp
)
target_compile_definitions(OtaSignatureKnownAnswerTest PRIVATE CROSSPOINT_HOST_TEST)
target_include_directories(OtaSignatureKnownAnswerTest PRIVATE ${REPO_ROOT}/lib/JsonParser ${REPO_ROOT} ${REPO_ROOT}/test/fixtures)
target_link_libraries(OtaSignatureKnownAnswerTest PRIVATE crosspoint_test_common GTest::gtest_main MbedTLS::mbedcrypto OpenSSL::Crypto)
gtest_discover_tests(OtaSignatureKnownAnswerTest)
