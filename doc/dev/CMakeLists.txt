# SPDX-FileCopyrightText: © 2019-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

set(DOXYGEN_AWESOME_CSS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/doxygen-awesome-css")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.cfg.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.cfg"
  @ONLY
)

set(extra_dev_docs
  "${CMAKE_SOURCE_DIR}/CONTRIBUTING.md"
  "${CMAKE_CURRENT_SOURCE_DIR}/mainpage.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/cyaml_schemas.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/gtk_tips.md"
  "${CMAKE_CURRENT_SOURCE_DIR}/processing_cycle.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/release_checklist.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/versioning.md"
  "${CMAKE_CURRENT_SOURCE_DIR}/weblate.h"
)

find_package(Doxygen)
find_program(DOT_EXECUTABLE dot)

if(DOXYGEN_FOUND AND DOT_EXECUTABLE)
  add_custom_target(doxygen-docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.cfg
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating developer documentation with Doxygen"
    DEPENDS ${extra_dev_docs} zrythm_gui_lib
    VERBATIM
  )
endif()
