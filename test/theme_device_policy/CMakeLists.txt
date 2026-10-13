foreach(profile IN ITEMS default ws397)
  set(target theme_device_policy_${profile}_test)
  add_executable(${target} ThemeDevicePolicyTest.cpp)
  target_include_directories(${target} PRIVATE
    ${REPO_ROOT}/src
    ${REPO_ROOT}/freeink-sdk/libs/hardware/BoardConfig/include
  )
  target_link_libraries(${target} PRIVATE crosspoint_test_common GTest::gtest_main)
  if(profile STREQUAL "ws397")
    target_compile_definitions(${target} PRIVATE FREEINK_DEVICE_WAVESHARE_EPAPER_397=1)
  endif()
  gtest_discover_tests(${target} TEST_PREFIX "${profile}.")
endforeach()
