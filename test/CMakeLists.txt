cmake_minimum_required(VERSION 3.16)
project(crosspoint_reader_tests C CXX)

if(DEFINED TLS_WOLFSSL_SHA384)
  set(TLS_WOLFSSL_SHA384 ${TLS_WOLFSSL_SHA384} CACHE BOOL "wolfSSL SHA-384 parity for tls_chain_verify" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
)

FetchContent_Declare(
  crosspoint_simulator
  GIT_REPOSITORY https://github.com/dezzw/crosspoint-simulator.git
  GIT_TAG 82727e8bbfa68456620041750f0d32326d1ca60a
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest crosspoint_simulator)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

execute_process(
  COMMAND python3 ${REPO_ROOT}/scripts/gen_i18n.py
  WORKING_DIRECTORY ${REPO_ROOT}
  RESULT_VARIABLE GEN_I18N_RESULT
)
if(NOT GEN_I18N_RESULT EQUAL 0)
  message(FATAL_ERROR "gen_i18n.py failed (exit ${GEN_I18N_RESULT})")
endif()

add_library(crosspoint_test_common INTERFACE)
target_include_directories(crosspoint_test_common INTERFACE
  ${REPO_ROOT}
  ${REPO_ROOT}/lib
)
target_compile_options(crosspoint_test_common INTERFACE
  -Wall
  -Wextra
  -pedantic
)

enable_testing()
include(GoogleTest)

add_subdirectory(streaming_json_parser)
add_subdirectory(release_json_parser)
add_subdirectory(semver)
add_subdirectory(firmware_version_tag)
add_subdirectory(firmware_board_tag)
add_subdirectory(crossmux_endpoints)
add_subdirectory(font_manifest_catalog)
add_subdirectory(ota_manifest)
add_subdirectory(ota_post_install)
add_subdirectory(ota_signature)
add_subdirectory(ota_signing_cross)
add_subdirectory(differential_rounding)
add_subdirectory(hyphenation_eval)
add_subdirectory(utf8_compose)
add_subdirectory(opds_filename)
add_subdirectory(html_to_plain_text)
add_subdirectory(minibidi_arabic)
add_subdirectory(combining_marks)
add_subdirectory(time_utils)
add_subdirectory(http_transport)
add_subdirectory(secure_http_client)
add_subdirectory(http_downloader)
add_subdirectory(tls_chain_verify)
add_subdirectory(weread_webapi)

add_subdirectory(sd_card_font_cache_format)
add_subdirectory(inx_navigation)
add_subdirectory(sloppy_digits)
add_subdirectory(book_cover_loader)
add_subdirectory(txt_page_index)
add_subdirectory(txt_paragraph)
add_subdirectory(txt_encoding)
add_subdirectory(txt_chapter_index)
add_subdirectory(credential_integrity)
add_subdirectory(footnote_list)
add_subdirectory(waveshare_function_button)
add_subdirectory(theme_device_policy)
add_subdirectory(keyboard_input_viewport)
add_subdirectory(sound_feedback)
add_subdirectory(gfx_two_bit_mapping)
add_subdirectory(gfx_outside_range_log)
add_subdirectory(dictionary_resource)
add_subdirectory(reading_guide_line)
add_subdirectory(png_conversion)
add_subdirectory(sample_row_clamp)
add_subdirectory(reading_background)
add_subdirectory(file_edit)
add_subdirectory(memory_safety)
add_subdirectory(xml_parser_utils)
add_subdirectory(token_boundary)
add_subdirectory(button_navigation)
add_subdirectory(bitmap_memory_source)
add_subdirectory(i18n_build_languages)
add_subdirectory(keyboard_layout_set)
add_subdirectory(system_settings_reset)
add_subdirectory(user_guide)
add_subdirectory(font_cache_manager)
add_subdirectory(css_parser)
add_subdirectory(chapter_html_slim_parser)
add_subdirectory(ligature_guard)
add_subdirectory(page_link)
add_subdirectory(ble_key_mapping)
add_subdirectory(ble_input)
add_subdirectory(reader_build_failure)
add_subdirectory(reader_font_preview)
add_subdirectory(reader_refresh)

# Font fallback reuses the existing Unicode mark classification.
foreach(font_test DifferentialRoundingTest LigatureGuardTest SdCardFontMemoryTest SdCardFontPagedTest)
  target_sources(${font_test} PRIVATE
    ${REPO_ROOT}/lib/MiniBidi/minibidi.c
    ${REPO_ROOT}/lib/MiniBidi/BidiUtils.cpp)
  target_include_directories(${font_test} PRIVATE
    ${REPO_ROOT}/lib/MiniBidi
    ${REPO_ROOT}/test/minibidi_arabic/stubs)
endforeach()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME WeReadLogout
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/scripts/tests/test_weread_logout.py)
add_test(NAME BuiltinFontShrink
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/scripts/tests/test_builtin_font_shrink.py)
add_test(NAME PioarduinoCache
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/scripts/tests/test_pioarduino_cache.py)
add_test(NAME ReadingUiRegressions
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/scripts/tests/test_reading_ui_regressions.py)
add_test(NAME MainTabEntryReleaseBarrier
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/scripts/tests/test_main_tab_entry_release_barrier.py)
add_test(NAME DownloadMemoryLifecycle
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/scripts/tests/test_download_memory_lifecycle.py)
add_test(NAME OtaPostInstallSourceGuards
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/scripts/tests/test_ota_post_install_sources.py)
add_test(NAME InxStyleCompatibility
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/test/inx_navigation/test_inx_style_compat.py)
add_test(NAME InxRecentThumbnails
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/test/inx_navigation/check_recent_thumbnails.py ${CMAKE_CXX_COMPILER})

add_test(NAME UiFontFallback
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/scripts/tests/test_ui_font_fallback.py)
add_test(NAME I18nBuildLanguages
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/scripts/tests/test_i18n_build_languages.py)
add_test(NAME WifiListNavRebuild
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/scripts/tests/test_wifi_list_nav_rebuild.py)
add_test(NAME OtaSigningKnownAnswerPython
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/scripts/tests/test_ota_signing_cross.py)
add_test(NAME TlsWolfSslLegacyBuildOffline
  WORKING_DIRECTORY ${REPO_ROOT}/scripts/tests
  COMMAND ${Python3_EXECUTABLE} -m unittest test_tls_wolfssl_legacy_build.TlsWolfSslOfflineTests -v)
add_test(NAME WolfSslSha384Patch
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/scripts/tests/test_wolfssl_sha384_patch.py)
add_test(NAME Ssd1677HostDriver
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/scripts/run_ssd1677_host_test.py)
