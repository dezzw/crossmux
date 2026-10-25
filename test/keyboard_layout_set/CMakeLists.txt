add_executable(KeyboardLayoutSetTest
  KeyboardLayoutSetTest.cpp
  ${REPO_ROOT}/src/activities/util/KeyboardLayoutSet.cpp
)

target_include_directories(KeyboardLayoutSetTest PRIVATE
  ${REPO_ROOT}/src/activities/util
  ${REPO_ROOT}/lib/I18n
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs
  ${REPO_ROOT}/src
  ${REPO_ROOT}/freeink-sdk/libs/ui/FreeInkUI/include
)

target_compile_definitions(KeyboardLayoutSetTest PRIVATE ENABLE_CHINESE_VERSION=1)

target_link_libraries(KeyboardLayoutSetTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(KeyboardLayoutSetTest)
