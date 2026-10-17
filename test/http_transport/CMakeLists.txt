add_executable(HttpTransportFailureTest
  HttpTransportFailureTest.cpp
  ${REPO_ROOT}/src/network/HttpTransportFailure.cpp
  ${REPO_ROOT}/src/network/HttpTransportFailureText.cpp
  ${REPO_ROOT}/lib/I18n/I18n.cpp
  ${REPO_ROOT}/lib/I18n/I18nStrings.cpp
)

target_compile_definitions(HttpTransportFailureTest PRIVATE CROSSPOINT_HOST_TEST)
target_include_directories(HttpTransportFailureTest PRIVATE
  stubs
  ${REPO_ROOT}/src
  ${REPO_ROOT}/src/network
  ${REPO_ROOT}/lib
  ${REPO_ROOT}/lib/hal
  ${REPO_ROOT}/lib/I18n
)
target_link_libraries(HttpTransportFailureTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(HttpTransportFailureTest)
