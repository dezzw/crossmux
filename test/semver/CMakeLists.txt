add_executable(SemVerCompareTest SemVerCompareTest.cpp ${REPO_ROOT}/lib/JsonParser/SemVerCompare.cpp)
target_link_libraries(SemVerCompareTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(SemVerCompareTest)
