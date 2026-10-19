set(REPO_ROOT ${CMAKE_SOURCE_DIR}/..)
add_executable(FirmwareBoardTagTest
  FirmwareBoardTagTest.cpp
  ${REPO_ROOT}/src/network/FirmwareBoardTag.cpp
)
target_compile_definitions(FirmwareBoardTagTest PRIVATE
  FREEINK_DEVICE_WAVESHARE_EPAPER_397=1
  CROSSPOINT_HOST_TEST=1
)
target_include_directories(FirmwareBoardTagTest PRIVATE ${REPO_ROOT})
target_link_libraries(FirmwareBoardTagTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(FirmwareBoardTagTest)
