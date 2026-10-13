add_executable(OtaManifestParserTest
  OtaManifestParserTest.cpp
  ${REPO_ROOT}/lib/JsonParser/OtaManifestParser.cpp
  ${REPO_ROOT}/lib/JsonParser/ReleaseJsonParser.cpp
  ${REPO_ROOT}/lib/JsonParser/StreamingJsonParser.cpp
)
target_include_directories(OtaManifestParserTest PRIVATE ${REPO_ROOT}/lib/JsonParser)
target_link_libraries(OtaManifestParserTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(OtaManifestParserTest)
