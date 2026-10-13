set(REPO_ROOT ${CMAKE_SOURCE_DIR}/..)
add_executable(FirmwareVersionTagTest
  FirmwareVersionTagTest.cpp
  ${REPO_ROOT}/src/network/FirmwareVersionTag.cpp
)
target_compile_definitions(FirmwareVersionTagTest PRIVATE CROSSPOINT_VERSION=\"1.6.0-test-version\")
target_include_directories(FirmwareVersionTagTest PRIVATE ${REPO_ROOT})
target_link_libraries(FirmwareVersionTagTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(FirmwareVersionTagTest)
