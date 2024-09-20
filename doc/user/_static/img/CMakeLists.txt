# SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# Find all PNG and SVG files in the current directory
file(GLOB_RECURSE PNG_SVG_FILES
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*.png" "*.svg")

# Copy PNG and SVG files to the build directory
foreach(FILE ${PNG_SVG_FILES})
  configure_file(
    ${FILE}
    ${CMAKE_CURRENT_BINARY_DIR}/${FILE}
    COPYONLY)
endforeach()

# Find all SVG files in the Zrythm dark icon theme directory
file(GLOB_RECURSE GUI_ICONS
  RELATIVE ${CMAKE_SOURCE_DIR}/data/icon-themes/zrythm-dark/scalable
  "${CMAKE_SOURCE_DIR}/data/icon-themes/zrythm-dark/scalable/*.svg")

# Copy GUI icons to the build directory
foreach(ICON ${GUI_ICONS})
  get_filename_component(icon_filename ${ICON} NAME)
  configure_file(
    ${CMAKE_SOURCE_DIR}/data/icon-themes/zrythm-dark/scalable/${ICON}
    ${CMAKE_CURRENT_BINARY_DIR}/${icon_filename}
    COPYONLY)
endforeach()
