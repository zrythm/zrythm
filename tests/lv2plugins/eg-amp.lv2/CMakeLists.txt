# SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

configure_file(
  manifest.ttl.in
  ${CMAKE_CURRENT_BINARY_DIR}/manifest.ttl
  @ONLY)

configure_file(
  amp.ttl
  ${CMAKE_CURRENT_BINARY_DIR}/amp.ttl
  COPYONLY)

add_library(eg_amp_lv2 MODULE
  amp.c)

set_target_properties(eg_amp_lv2 PROPERTIES
  PREFIX ""
  OUTPUT_NAME "amp")

target_link_libraries(eg_amp_lv2 PRIVATE
  zrythm::lv2)

list(APPEND test_lv2_plugin_libs eg_amp_lv2)
list(APPEND test_lv2_plugins
  "eg_amp|http://lv2plug.in/plugins/eg-amp|${CMAKE_CURRENT_BINARY_DIR}|eg_amp_lv2")

set(test_lv2_plugin_libs ${test_lv2_plugin_libs} PARENT_SCOPE)
set(test_lv2_plugins ${test_lv2_plugins} PARENT_SCOPE)
