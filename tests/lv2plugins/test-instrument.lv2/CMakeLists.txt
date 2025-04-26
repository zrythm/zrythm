# SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

configure_file(
  manifest.ttl.in
  ${CMAKE_CURRENT_BINARY_DIR}/manifest.ttl
  @ONLY)

configure_file(
  test-instrument.ttl
  ${CMAKE_CURRENT_BINARY_DIR}/test-instrument.ttl
  COPYONLY)

add_library(test_instrument_lv2 MODULE
  test-instrument.c)

set_target_properties(test_instrument_lv2 PROPERTIES
  PREFIX ""
  OUTPUT_NAME "test-instrument")

target_link_libraries(test_instrument_lv2 PRIVATE
  zrythm::lv2)

list(APPEND test_lv2_plugin_libs test_instrument_lv2)
list(APPEND test_lv2_plugins
  "test_instrument|https://lv2.zrythm.org/test-instrument|${CMAKE_CURRENT_BINARY_DIR}|test_instrument_lv2")

set(test_lv2_plugin_libs ${test_lv2_plugin_libs} PARENT_SCOPE)
set(test_lv2_plugins ${test_lv2_plugins} PARENT_SCOPE)
