if(NOT DEFINED TLS_WOLFSSL_SHA384)
  option(TLS_WOLFSSL_SHA384 "Enable wolfSSL SHA-384/512 (device parity)" ON)
endif()
if(NOT DEFINED TLS_WOLFSSL_ALT_CHAINS)
  option(TLS_WOLFSSL_ALT_CHAINS "Enable WOLFSSL_ALT_CERT_CHAINS (device parity)" ON)
endif()

set(WOLFSSL_USER_SETTINGS ON CACHE BOOL "" FORCE)
set(WOLFSSL_TLS13 ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(WOLFSSL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WOLFSSL_CRYPT_TESTS OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
  wolfssl
  GIT_REPOSITORY https://github.com/wolfSSL/wolfssl.git
  GIT_TAG v5.7.2-stable
)
FetchContent_MakeAvailable(wolfssl)

if(TLS_WOLFSSL_SHA384)
  set(_crossmux_sha384 1)
else()
  set(_crossmux_sha384 0)
endif()
if(TLS_WOLFSSL_ALT_CHAINS)
  set(_crossmux_alt 1)
else()
  set(_crossmux_alt 0)
endif()

if(TARGET wolfssl)
  target_include_directories(wolfssl PRIVATE ${REPO_ROOT}/scripts/tls)
  target_compile_options(wolfssl PRIVATE -Wno-error)
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(wolfssl PRIVATE -Wno-error=stringop-overflow -Wno-stringop-overflow)
  endif()
  target_compile_definitions(wolfssl PRIVATE
    CROSSMUX_WOLFSSL_ALT_CERT_CHAINS=${_crossmux_alt}
  )
  if(NOT TLS_WOLFSSL_SHA384)
    target_compile_definitions(wolfssl PRIVATE CROSSMUX_WOLFSSL_DISABLE_SHA384=1 NO_SHA384 NO_SHA512)
  endif()
endif()

add_executable(TlsCaChainVerifyTest TlsCaChainVerifyTest.cpp)
target_include_directories(TlsCaChainVerifyTest PRIVATE ${REPO_ROOT}/src/network ${REPO_ROOT}/scripts/tls ${wolfssl_SOURCE_DIR})
target_link_libraries(TlsCaChainVerifyTest PRIVATE crosspoint_test_common GTest::gtest_main wolfssl)
target_compile_definitions(TlsCaChainVerifyTest PRIVATE
  REPO_ROOT="${REPO_ROOT}"
  TLS_WOLFSSL_SHA384_ENABLED=${_crossmux_sha384}
  TLS_WOLFSSL_ALT_CHAINS_ENABLED=${_crossmux_alt}
)
gtest_discover_tests(TlsCaChainVerifyTest)

add_executable(TlsHostVerify TlsHostVerify.cpp)
target_include_directories(TlsHostVerify PRIVATE ${REPO_ROOT}/src/network ${REPO_ROOT}/scripts/tls ${wolfssl_SOURCE_DIR})
target_link_libraries(TlsHostVerify PRIVATE wolfssl)
target_compile_definitions(TlsHostVerify PRIVATE REPO_ROOT="${REPO_ROOT}")

add_test(
  NAME TlsHostVerifyGithub
  COMMAND TlsHostVerify github.com ${REPO_ROOT}/src/network/TlsCaBundle.pem
)
add_test(
  NAME TlsHostVerifyCrossmux
  COMMAND TlsHostVerify crossmux.com ${REPO_ROOT}/src/network/TlsCaBundle.pem
)
set_tests_properties(TlsHostVerifyGithub TlsHostVerifyCrossmux PROPERTIES LABELS "network")
