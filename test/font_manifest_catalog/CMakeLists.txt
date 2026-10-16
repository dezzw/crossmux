include(FetchContent)
FetchContent_Declare(
  ArduinoJson
  GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git
  GIT_TAG v7.4.2
)
FetchContent_MakeAvailable(ArduinoJson)

add_executable(FontManifestCatalogTest
  FontManifestCatalogTest.cpp
  ${REPO_ROOT}/src/network/FontManifestCatalog.cpp
  ${REPO_ROOT}/src/FontInstaller.cpp
)

target_compile_definitions(FontManifestCatalogTest PRIVATE CROSSPOINT_HOST_TEST)
target_include_directories(FontManifestCatalogTest PRIVATE
  stubs
  ${REPO_ROOT}
  ${REPO_ROOT}/src
  ${REPO_ROOT}/lib
  ${REPO_ROOT}/lib/hal
)
target_link_libraries(FontManifestCatalogTest PRIVATE crosspoint_test_common GTest::gtest_main ArduinoJson)
gtest_discover_tests(FontManifestCatalogTest)
