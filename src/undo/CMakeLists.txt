# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_undo_lib STATIC)

target_sources(zrythm_undo_lib
  PRIVATE
    undo_stack.h
    undo_stack.cpp
)

set_target_properties(zrythm_undo_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_precompile_headers(zrythm_undo_lib PUBLIC
  $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/undo_stack.h>)

target_link_libraries(zrythm_undo_lib
  PUBLIC
    ${zrythm_link_libs}
    zrythm_commands_lib
    zrythm::include_dirs
    zrythm::all_compile_options
)

# create a plugin so that types get exposed to QML
qt_add_qml_module(zrythm_undo_lib
  URI ZrythmUndo
  VERSION 1.0
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmUndo
)
