set(HYPHENATION_EVAL_LIB_SOURCES
  ${REPO_ROOT}/lib/Epub/Epub/hyphenation/Hyphenator.cpp
  ${REPO_ROOT}/lib/Epub/Epub/hyphenation/LanguageRegistry.cpp
  ${REPO_ROOT}/lib/Epub/Epub/hyphenation/LiangHyphenation.cpp
  ${REPO_ROOT}/lib/Epub/Epub/hyphenation/HyphenationCommon.cpp
  ${REPO_ROOT}/lib/Utf8/Utf8.cpp
)

set(HYPHENATION_EVAL_SOURCES
  HyphenationEvaluationTest.cpp
  NullHyphenatorFallbackTest.cpp
  HyphenatorSegmentPatternTest.cpp
  ${HYPHENATION_EVAL_LIB_SOURCES}
)

set(HYPHENATION_EVAL_INCLUDES
  ${REPO_ROOT}/lib/Epub
  ${REPO_ROOT}/lib/Utf8
)

add_executable(HyphenationEvaluationTest ${HYPHENATION_EVAL_SOURCES})

target_include_directories(HyphenationEvaluationTest PRIVATE ${HYPHENATION_EVAL_INCLUDES})

target_compile_definitions(HyphenationEvaluationTest PRIVATE
  HYPHENATION_RESOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
)

target_link_libraries(HyphenationEvaluationTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(HyphenationEvaluationTest PROPERTIES LABELS hyphenation_default)

# Compile LanguageRegistry with all language tries linked (guarded #ifdef path).
add_executable(LanguageRegistryAllLangsTest
  LanguageRegistryAllLangsTest.cpp
  ${REPO_ROOT}/lib/Epub/Epub/hyphenation/LanguageRegistry.cpp
  ${REPO_ROOT}/lib/Epub/Epub/hyphenation/HyphenationCommon.cpp
  ${REPO_ROOT}/lib/Utf8/Utf8.cpp
)

target_include_directories(LanguageRegistryAllLangsTest PRIVATE ${HYPHENATION_EVAL_INCLUDES})

target_compile_definitions(LanguageRegistryAllLangsTest PRIVATE
  CROSSPOINT_HYPHENATION_ALL_LANGUAGES
)

target_link_libraries(LanguageRegistryAllLangsTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(LanguageRegistryAllLangsTest PROPERTIES LABELS hyphenation_all_langs)

add_executable(HyphenationEvaluationAllLangsTest
  HyphenationEvaluationTest.cpp
  ${HYPHENATION_EVAL_LIB_SOURCES}
)

target_include_directories(HyphenationEvaluationAllLangsTest PRIVATE ${HYPHENATION_EVAL_INCLUDES})

target_compile_definitions(HyphenationEvaluationAllLangsTest PRIVATE
  CROSSPOINT_HYPHENATION_ALL_LANGUAGES
  HYPHENATION_RESOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
)

target_link_libraries(HyphenationEvaluationAllLangsTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(HyphenationEvaluationAllLangsTest
  PROPERTIES LABELS hyphenation_all_langs
  TEST_PREFIX "AllLangs."
)
