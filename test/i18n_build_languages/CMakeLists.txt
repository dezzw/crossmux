add_executable(I18nBuildLanguagesTest
  I18nBuildLanguagesTest.cpp
  ${REPO_ROOT}/lib/I18n/I18n.cpp
  ${REPO_ROOT}/lib/I18n/I18nStrings.cpp
)

target_include_directories(I18nBuildLanguagesTest PRIVATE
  ${REPO_ROOT}/lib/I18n
)

target_compile_definitions(I18nBuildLanguagesTest PRIVATE ENABLE_CHINESE_VERSION=1)

target_link_libraries(I18nBuildLanguagesTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(I18nBuildLanguagesTest)
