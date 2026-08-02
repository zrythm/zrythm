# SPDX-FileCopyrightText: © 2019-2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if(NOT ZRYTHM_TESTS)
  return()
endif()

include(GoogleTest)
function(zrythm_discover_tests TARGET_NAME)
  # Disable per-target QT_TESTCASE_SOURCEDIR/BUILDDIR so that test targets with
  # identical precompiled header content can reuse each other's PCH via
  # REUSE_FROM (the compiler rejects PCH reuse when macros differ).
  # No tests use QFINDTESTDATA() so these values are unused.
  set_target_properties(${TARGET_NAME} PROPERTIES
    QT_SKIP_DEFAULT_TESTCASE_DIRS TRUE
  )
  # see https://discourse.cmake.org/t/googletest-crash-when-using-cmake-xcode-arm64/5766/11
  # Ad-hoc signing is required on Apple Silicon to run freshly-built test
  # binaries (gtest_discover_tests executes them at build time). The Xcode
  # generator doesn't auto-sign, and even Ninja builds benefit from a clean
  # re-sign after linking against conan-provided dylibs.
  if(APPLE)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
      COMMAND codesign -f -s - "$<TARGET_FILE:${TARGET_NAME}>"
      COMMENT "Ad-hoc signing ${TARGET_NAME} POST_BUILD"
    )
  endif()

  if(WIN32)
    gtest_add_tests(TARGET ${TARGET_NAME})
  else()
    gtest_discover_tests(${TARGET_NAME}
      # Default of 5s is too short on mac; the QML test binary pulls in the
      # largest Qt dylibs (Quick/Qml) whose cold-start (dyld load + signature
      # validation) can exceed 10s on a loaded macOS runner.
      DISCOVERY_TIMEOUT 60
      XML_OUTPUT_DIR "${CMAKE_BINARY_DIR}/test_results"
    )
  endif()
endfunction()

add_library(zrythm_test_helpers_lib STATIC)

target_sources(zrythm_test_helpers_lib
  PUBLIC
    FILE_SET HEADERS
    FILES
      helpers/test_plugin_finder.h
      helpers/in_memory_settings_backend.h
      helpers/mock_hardware_audio_interface.h
      helpers/mock_hardware_audio_interface_threaded.h
      helpers/mock_plugin_host_window.h
      helpers/mock_qobject.h
      helpers/mock_uuid_object.h
      helpers/mock_settings_backend.h
      helpers/project_fixture.h
      helpers/project_json_comparators.h
      helpers/qt_helpers.h
      helpers/scoped_qcoreapplication.h
      helpers/scoped_juce_qapplication.h
)
target_link_libraries(zrythm_test_helpers_lib
  PUBLIC
    GTest::gmock_main
    zrythm::include_dirs
    ${zrythm_link_libs}
    zrythm::all_compile_options
    zrythm_utils_lib
    Qt6::Test
)
target_include_directories(zrythm_test_helpers_lib
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(zrythm_test_helpers_lib
  INTERFACE
    TEST_WAV_FILE_PATH="${CMAKE_SOURCE_DIR}/tests/test.wav"
)

target_precompile_headers(zrythm_test_helpers_lib
  PUBLIC
    $<$<COMPILE_LANGUAGE:CXX>:<gtest/gtest.h>>
    $<$<COMPILE_LANGUAGE:CXX>:<gmock/gmock.h>>
    $<$<COMPILE_LANGUAGE:CXX>:<QSignalSpy>>
    $<$<COMPILE_LANGUAGE:CXX>:"dsp/tempo_map.h">
)

set_target_properties(zrythm_test_helpers_lib PROPERTIES
  VERIFY_INTERFACE_HEADER_SETS ${ZRYTHM_VERIFY_INTERFACE_HEADER_SETS}
)

set(test_lv2_plugins)
set(test_lv2_plugin_libs)
add_subdirectory(lv2plugins)

add_subdirectory(unit)
add_subdirectory(benchmarks)

add_subdirectory(test_plugins)

# Pass the plugin search paths to the tests
# Derive the VST3 search path from the SDK's own SMTG_PLUGIN_PACKAGE_PATH
# property: on GNU/Linux and macOS Ninja the bundles are at VST3/<config>/, but
# on Windows with SMTG_CUSTOM_BINARY_LOCATION they are directly at VST3/.
get_target_property(vst3_pkg_path test_gain_vst3 SMTG_PLUGIN_PACKAGE_PATH)
get_filename_component(vst3_search_path "${vst3_pkg_path}" DIRECTORY)
list(LENGTH zrythm_test_vst3_fixture_targets vst3_plugins_count)
# One bundle per fixture, except the synth bundle which contains both note
# dialect variants of the synth (2 plugins)
list(LENGTH zrythm_test_clap_fixture_targets clap_plugins_count)
math(EXPR clap_plugins_count "${clap_plugins_count} + 1")
target_compile_definitions(zrythm_test_helpers_lib
  PUBLIC
    TEST_VST3_SEARCH_PATHS="${vst3_search_path}"
    TEST_CLAP_SEARCH_PATHS="${CMAKE_CURRENT_BINARY_DIR}/test_plugins/CLAP/$<CONFIG>"
    TEST_VST3_PLUGINS_COUNT=${vst3_plugins_count}
    TEST_CLAP_PLUGINS_COUNT=${clap_plugins_count}
)

# Ensure test plugins are built before running tests
add_dependencies(zrythm_test_helpers_lib zrythm_test_plugins)

add_subdirectory(integration)
