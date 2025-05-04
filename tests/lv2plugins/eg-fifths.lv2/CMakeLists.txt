# SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

configure_file(
  manifest.ttl.in
  ${CMAKE_CURRENT_BINARY_DIR}/manifest.ttl
  @ONLY)

configure_file(
  fifths.ttl
  ${CMAKE_CURRENT_BINARY_DIR}/fifths.ttl
  COPYONLY)

add_library(eg_fifths_lv2 MODULE
  fifths.c)

set_target_properties(eg_fifths_lv2 PROPERTIES
  PREFIX ""
  OUTPUT_NAME "fifths"
  AUTOMOC OFF
)

target_link_libraries(eg_fifths_lv2 PRIVATE
  zrythm::lv2)
