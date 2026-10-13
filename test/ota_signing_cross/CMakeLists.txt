set(REPO_ROOT ${CMAKE_SOURCE_DIR}/..)
find_package(OpenSSL REQUIRED)
add_executable(OtaSigningCrossTest
  OtaSigningCrossTest.cpp
  ${REPO_ROOT}/lib/JsonParser/OtaSigningPayload.cpp
  ${REPO_ROOT}/lib/JsonParser/OtaSignatureVerify.cpp
)
target_compile_definitions(OtaSigningCrossTest PRIVATE CROSSPOINT_HOST_TEST)
target_include_directories(OtaSigningCrossTest PRIVATE ${REPO_ROOT}/lib/JsonParser ${REPO_ROOT})
target_link_libraries(OtaSigningCrossTest PRIVATE crosspoint_test_common GTest::gtest_main OpenSSL::SSL OpenSSL::Crypto)
gtest_discover_tests(OtaSigningCrossTest)
