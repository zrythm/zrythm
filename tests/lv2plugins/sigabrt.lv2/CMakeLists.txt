# SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

configure_file(
  manifest.ttl.in
  ${CMAKE_CURRENT_BINARY_DIR}/manifest.ttl
  @ONLY)

configure_file(
  sigabrt.ttl
  ${CMAKE_CURRENT_BINARY_DIR}/sigabrt.ttl
  COPYONLY)

add_library(sigabrt_lv2 MODULE
  sigabrt.c)

set_target_properties(sigabrt_lv2 PROPERTIES
  PREFIX ""
  OUTPUT_NAME "sigabrt")

target_link_libraries(sigabrt_lv2 PRIVATE
  zrythm::lv2)

list(APPEND test_lv2_plugin_libs sigabrt_lv2)
list(APPEND test_lv2_plugins
  "sigabrt|https://www.zrythm.org/plugins/sigabrt|${CMAKE_CURRENT_BINARY_DIR}|sigabrt_lv2")

set(test_lv2_plugin_libs ${test_lv2_plugin_libs} PARENT_SCOPE)
set(test_lv2_plugins ${test_lv2_plugins} PARENT_SCOPE)
