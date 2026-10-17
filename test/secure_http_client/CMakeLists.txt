add_executable(SecureHttpClientTest
  SecureHttpClientTest.cpp
  ${REPO_ROOT}/crossmux-patches/SecureNet/src/SecureClient.cpp
)

target_compile_definitions(SecureHttpClientTest PRIVATE CROSSPOINT_SECURE_HTTP_CLIENT_TEST)
target_include_directories(SecureHttpClientTest PRIVATE
  stubs
  ${REPO_ROOT}/crossmux-patches/SecureNet/include
  ${REPO_ROOT}/test/secure_http_client
)
target_link_libraries(SecureHttpClientTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(SecureHttpClientTest)

add_executable(SecureClientReadTest
  SecureClientReadTest.cpp
  ${REPO_ROOT}/crossmux-patches/SecureNet/src/SecureClient.cpp
)

target_compile_definitions(SecureClientReadTest PRIVATE CROSSPOINT_SECURECLIENT_HOST_TEST)
target_include_directories(SecureClientReadTest PRIVATE
  stubs
  ${REPO_ROOT}/crossmux-patches/SecureNet/include
)
target_link_libraries(SecureClientReadTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(SecureClientReadTest)
