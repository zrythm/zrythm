# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_controllers_lib STATIC)

target_sources(zrythm_controllers_lib
  PRIVATE
    transport_controller.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ".."
    FILES
      transport_controller.h
)

target_include_directories(zrythm_controllers_lib
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
)

set_target_properties(zrythm_controllers_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
  VERIFY_INTERFACE_HEADER_SETS ${ZRYTHM_VERIFY_INTERFACE_HEADER_SETS}
)

target_link_libraries(zrythm_controllers_lib
  PUBLIC
    ${zrythm_link_libs}
    zrythm_undo_lib
    zrythm::include_dirs
    zrythm::all_compile_options
)

# create a plugin so that types get exposed to QML
qt_add_qml_module(zrythm_controllers_lib
  URI ZrythmControllers
  VERSION 1.0
  IMPORTS
    TARGET zrythm_dsp_lib
    TARGET zrythm_undo_lib
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmControllers
)
