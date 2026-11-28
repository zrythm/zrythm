# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# Path of the application the tests start. A CI job can set this to a
# packaged application.
set(ZRYTHM_E2E_APP_PATH "$<TARGET_FILE:zrythm>" CACHE STRING
  "Application started by the end-to-end tests")

add_executable(zrythm_e2e_tests
  e2e_test_main.cpp
  command_line_test.cpp
  startup_test.cpp
  new_project_test.cpp
  user_data_dir_test.cpp
  launcher.h
  mcp_client.h
)

set_target_properties(zrythm_e2e_tests PROPERTIES
  AUTOMOC OFF
)

target_link_libraries(zrythm_e2e_tests PRIVATE
  GTest::gtest
  Qt6::Core
  Qt6::Network
  zrythm::all_compile_options
  zrythm_test_warning_suppressions_link_last
)

target_compile_definitions(zrythm_e2e_tests PRIVATE
  ZRYTHM_E2E_APP_PATH="${ZRYTHM_E2E_APP_PATH}"
  ZRYTHM_E2E_APP_VERSION="${PACKAGE_VERSION}"
)
if(WIN32)
  # The build tree holds only the Windows platform plugin; the tests give
  # the application the offscreen plugin through QT_PLUGIN_PATH
  target_compile_definitions(zrythm_e2e_tests PRIVATE
    ZRYTHM_E2E_QT_PLUGINS_DIR="${QT6_INSTALL_PREFIX}/${QT6_INSTALL_PLUGINS}"
  )
endif()

add_dependencies(zrythm_e2e_tests zrythm)

# Every test starts the application, so the tests take longer than unit
# tests and must not run at the same time
zrythm_discover_tests(zrythm_e2e_tests
  TEST_PREFIX "zrythm_e2e_tests."
  PROPERTIES RUN_SERIAL TRUE TIMEOUT 120
)
