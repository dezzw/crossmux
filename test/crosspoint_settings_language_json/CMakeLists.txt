include(FetchContent)
FetchContent_Declare(
  ArduinoJson
  GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git
  GIT_TAG v7.4.2
)
FetchContent_MakeAvailable(ArduinoJson)

configure_file(${REPO_ROOT}/src/CrossPointSettings.cpp ${CMAKE_CURRENT_BINARY_DIR}/CrossPointSettings.cpp COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/stubs/SettingsList.h ${CMAKE_CURRENT_BINARY_DIR}/SettingsList.h COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/stubs/ObfuscationUtils.h ${CMAKE_CURRENT_BINARY_DIR}/ObfuscationUtils.h COPYONLY)

add_executable(CrossPointSettingsLanguageJsonTest
  CrossPointSettingsLanguageJsonTest.cpp
  PersistableStoreHost.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/CrossPointSettings.cpp
  ${REPO_ROOT}/lib/I18n/I18n.cpp
  ${REPO_ROOT}/lib/I18n/I18nStrings.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs/ReaderFontSizesHost.cpp
)

target_compile_definitions(CrossPointSettingsLanguageJsonTest PRIVATE
  ENABLE_CHINESE_VERSION=1
  CROSSPOINT_HOST_TEST=1
  FREEINK_DEVICE_WAVESHARE_EPAPER_397=1
  CROSSPOINT_EMULATED=1
)

target_include_directories(CrossPointSettingsLanguageJsonTest BEFORE PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs
  ${REPO_ROOT}/src
  ${REPO_ROOT}/lib
  ${REPO_ROOT}/lib/I18n
  ${REPO_ROOT}/lib/Serialization
  ${REPO_ROOT}/lib/hal
  ${REPO_ROOT}/lib/Epub
  ${REPO_ROOT}/lib/GfxRenderer
)

target_link_libraries(CrossPointSettingsLanguageJsonTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
  ArduinoJson
)

# Executed via scripts/tests/test_crosspoint_settings_language_resave.py (ctest add_test).
