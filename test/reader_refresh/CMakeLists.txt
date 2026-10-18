add_executable(ReaderRefreshCadenceTest ReaderRefreshCadenceTest.cpp)
target_include_directories(ReaderRefreshCadenceTest PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs
  ${REPO_ROOT}/src/activities/reader
)
target_link_libraries(ReaderRefreshCadenceTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(ReaderRefreshCadenceTest)
