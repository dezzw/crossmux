add_executable(HttpDownloaderDownloadToFileTest
  HttpDownloaderDownloadToFileTest.cpp
  ${REPO_ROOT}/src/network/HttpDownloader.cpp
  ${REPO_ROOT}/src/network/HttpTransportFailure.cpp
)

target_compile_definitions(HttpDownloaderDownloadToFileTest PRIVATE
  CROSSPOINT_HOST_TEST
  CROSSPOINT_HTTP_DOWNLOADER_MOCK_WOLF=1
  CROSSPOINT_VERSION=\"host-test\"
)
target_include_directories(HttpDownloaderDownloadToFileTest BEFORE PRIVATE
  stubs
  ${REPO_ROOT}/src
  ${REPO_ROOT}/src/network
  ${REPO_ROOT}/lib/hal
)
target_link_libraries(HttpDownloaderDownloadToFileTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(HttpDownloaderDownloadToFileTest)
