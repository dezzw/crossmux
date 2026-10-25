include(${CMAKE_CURRENT_LIST_DIR}/../cmake/FindMbedTLS.cmake)
find_package(OpenSSL REQUIRED)

add_executable(OtaSigningCrossTest
  OtaSigningCrossTest.cpp
  ${REPO_ROOT}/lib/JsonParser/OtaSigningPayload.cpp
  ${REPO_ROOT}/lib/JsonParser/OtaSignatureVerify.cpp
)
target_compile_definitions(OtaSigningCrossTest PRIVATE
  CROSSPOINT_HOST_TEST
  CROSSPOINT_OTA_TEST_PRIVATE_KEY_PATH="${OTA_TEST_FIXTURE_DIR}/ota_test_private.pem")
target_include_directories(OtaSigningCrossTest PRIVATE
  ${REPO_ROOT}/lib/JsonParser ${REPO_ROOT} ${OTA_TEST_FIXTURE_DIR})
target_link_libraries(OtaSigningCrossTest PRIVATE crosspoint_test_common GTest::gtest_main MbedTLS::mbedcrypto OpenSSL::SSL OpenSSL::Crypto)
gtest_discover_tests(OtaSigningCrossTest)
