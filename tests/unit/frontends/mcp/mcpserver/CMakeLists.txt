# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_frontends_mcp_mcpserver_unit_tests
  mcp_server_test.cpp
)

set_target_properties(zrythm_frontends_mcp_mcpserver_unit_tests PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_frontends_mcp_mcpserver_unit_tests PRIVATE
  GTest::gmock_main
  Qt6::Network
  zrythm::mcp_server_lib
  zrythm_test_helpers_lib
  zrythm_test_warning_suppressions_link_last
)

zrythm_discover_tests(zrythm_frontends_mcp_mcpserver_unit_tests)
