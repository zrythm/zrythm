# SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

configure_file(
  manifest.ttl
  ${CMAKE_CURRENT_BINARY_DIR}/manifest.ttl
  @ONLY)

configure_file(
  plumbing.ttl
  ${CMAKE_CURRENT_BINARY_DIR}/plumbing.ttl
  @ONLY)

add_library(plumbing_lv2 MODULE
  midieat.c
  plumbing.c
  route.c)

set_target_properties(plumbing_lv2 PROPERTIES
  PREFIX ""
  OUTPUT_NAME "plumbing"
  UNITY_BUILD OFF
)

target_link_libraries(plumbing_lv2 PRIVATE
  zrythm::lv2)

list(APPEND test_lv2_plugin_libs plumbing_lv2)
list(APPEND test_lv2_plugins
  "plumbing|http://lv2plug.in/plugins/plumbing|${CMAKE_CURRENT_BINARY_DIR}|plumbing_lv2")

set(test_lv2_plugin_libs ${test_lv2_plugin_libs} PARENT_SCOPE)
set(test_lv2_plugins ${test_lv2_plugins} PARENT_SCOPE)
