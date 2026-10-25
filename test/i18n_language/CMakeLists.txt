add_executable(I18nLanguageTest
  I18nLanguageTest.cpp
  ${REPO_ROOT}/lib/I18n/I18n.cpp
  ${REPO_ROOT}/lib/I18n/I18nStrings.cpp
)
target_include_directories(I18nLanguageTest PRIVATE ${REPO_ROOT}/lib/I18n)
target_link_libraries(I18nLanguageTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(I18nLanguageTest)
