add_executable(ota_post_install_test
  OtaPostInstallTest.cpp
  HalOtaSlotLinkStub.cpp
  ${REPO_ROOT}/src/OtaRunningImageConfirm.cpp
)
target_include_directories(ota_post_install_test BEFORE PRIVATE
  ${REPO_ROOT}/test/ota_post_install/stubs
)
target_include_directories(ota_post_install_test PRIVATE
  ${REPO_ROOT}/src
  ${REPO_ROOT}/lib
  ${REPO_ROOT}/lib/hal
  ${REPO_ROOT}/lib/Logging
)
target_compile_definitions(ota_post_install_test PRIVATE SIMULATOR=1)
target_link_libraries(ota_post_install_test PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(ota_post_install_test)
