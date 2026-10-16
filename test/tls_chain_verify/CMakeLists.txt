if(NOT DEFINED TLS_WOLFSSL_SHA384)
  option(TLS_WOLFSSL_SHA384 "Enable wolfSSL SHA-384/512 (device parity)" ON)
endif()

include(FetchContent)
set(WOLFSSL_SHA384 ${TLS_WOLFSSL_SHA384} CACHE BOOL "" FORCE)
set(WOLFSSL_SHA512 ${TLS_WOLFSSL_SHA384} CACHE BOOL "" FORCE)
set(WOLFSSL_SP_384 ${TLS_WOLFSSL_SHA384} CACHE BOOL "" FORCE)
set(WOLFSSL_TLS13 ON CACHE BOOL "" FORCE)
set(WOLFSSL_USER_SETTINGS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(WOLFSSL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WOLFSSL_CRYPT_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  wolfssl
  GIT_REPOSITORY https://github.com/wolfSSL/wolfssl.git
  GIT_TAG v5.7.2-stable
)
FetchContent_MakeAvailable(wolfssl)

# wolfSSL 5.7.x triggers -Werror=stringop-overflow on GCC 13+ CI when SHA-384 is enabled.
if(TARGET wolfssl)
  target_compile_options(wolfssl PRIVATE -Wno-error=stringop-overflow -Wno-stringop-overflow)
endif()

add_executable(TlsCaChainVerifyTest TlsCaChainVerifyTest.cpp)
target_include_directories(TlsCaChainVerifyTest PRIVATE ${REPO_ROOT}/src/network ${wolfssl_SOURCE_DIR})
target_link_libraries(TlsCaChainVerifyTest PRIVATE crosspoint_test_common GTest::gtest_main wolfssl)
target_compile_definitions(TlsCaChainVerifyTest PRIVATE REPO_ROOT="${REPO_ROOT}")
if(TLS_WOLFSSL_SHA384)
  target_compile_definitions(TlsCaChainVerifyTest PRIVATE TLS_WOLFSSL_SHA384_ENABLED=1)
else()
  target_compile_definitions(TlsCaChainVerifyTest PRIVATE TLS_WOLFSSL_SHA384_ENABLED=0)
endif()
gtest_discover_tests(TlsCaChainVerifyTest)
