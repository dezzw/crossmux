add_executable(HttpTransportFailureTest
  HttpTransportFailureTest.cpp
  ${REPO_ROOT}/src/network/HttpTransportFailure.cpp
)

target_compile_definitions(HttpTransportFailureTest PRIVATE CROSSPOINT_HOST_TEST)
target_include_directories(HttpTransportFailureTest PRIVATE
  stubs
  ${REPO_ROOT}/src
  ${REPO_ROOT}/src/network
  ${REPO_ROOT}/lib
  ${REPO_ROOT}/lib/hal
)
target_link_libraries(HttpTransportFailureTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(HttpTransportFailureTest)
