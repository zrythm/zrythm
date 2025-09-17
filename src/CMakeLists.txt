# SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_gui_lib STATIC)

# each executable/library has its own subdirectory
add_subdirectory(utils)
add_subdirectory(dsp)
add_subdirectory(plugins)
add_subdirectory(structure)
add_subdirectory(commands)
add_subdirectory(undo)
add_subdirectory(actions)

target_include_directories(zrythm_gui_lib
  PUBLIC
    # C++ headers in QML modules must be available as toplevel includes
    # https://bugreports.qt.io/browse/QTBUG-87221
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/backend
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/backend/backend
)

# create a plugin so that types get exposed to QML
# Note: this needs to be in the same directory as the qt_add_library() call
qt_add_qml_module(zrythm_gui_lib
  URI ZrythmGui
  VERSION 1.0
  DEPENDENCIES
    QtCore # Needed for QAbstractItemModel-derived types
  IMPORTS
    TARGET zrythm_dsp_lib
    TARGET zrythm_arrangement_lib
    TARGET zrythm_structure_tracks_lib
    TARGET zrythm_plugins_lib
    TARGET zrythm_commands_lib
    TARGET zrythm_undo_lib
    TARGET zrythm_actions_lib
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmGui
)

add_subdirectory(gui)
add_subdirectory(engine)

target_sources(zrythm_gui_lib PRIVATE globals.cpp)
if(MSVC)
  # some files are too big - this increases the section limit in object files
  target_compile_options(zrythm_gui_lib PRIVATE /bigobj)
endif()

if(WIN32)
  # There are problems if added to zrythm_srcs directly
  target_sources(zrythm_gui_lib PRIVATE ${win_res})
endif()

set_target_properties(zrythm_gui_lib PROPERTIES
  OUTPUT_NAME zrythm_gui_lib
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)
target_link_libraries(zrythm_gui_lib
  PUBLIC
    zrythm_actions_lib
    zrythm::include_dirs
    zrythm::all_compile_options
)
target_precompile_headers(zrythm_gui_lib PUBLIC
  $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/gui/backend/backend/zrythm.h>)

# add resource file with icon
if(WIN32)
  set(_rc_file "${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc")
  file(WRITE ${_rc_file} "IDI_ICON1 ICON \"${ZRYTHM_ICO_FILE}\"\n")
  target_sources(zrythm PRIVATE ${_rc_file})
endif()

target_sources(zrythm PRIVATE main.cpp)
target_link_libraries(zrythm PRIVATE zrythm_gui_lib)
target_precompile_headers(zrythm PUBLIC
  $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/gui/backend/zrythm_application.h>)

if(OS_GNU)
  add_dependencies(zrythm appdata-xml desktop-file)
endif()

if(WIN32)
  if(MSVC)
    target_link_options(zrythm PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
  endif()
endif()

add_subdirectory(engine-process)
add_dependencies(zrythm zrythm-engine plugin-scanner)

install(
  TARGETS
    zrythm
    plugin-scanner
  RUNTIME
    COMPONENT Runtime
    # put helper executables inside the main zrythm app bundle on MacOS
    DESTINATION $<IF:$<PLATFORM_ID:Darwin>,$<TARGET_BUNDLE_DIR_NAME:zrythm>/Contents/MacOS,${CMAKE_INSTALL_BINDIR}>
  BUNDLE
    COMPONENT Runtime
    DESTINATION .
  LIBRARY
    COMPONENT Runtime
    NAMELINK_COMPONENT Development
  ARCHIVE
    COMPONENT Development
    DESTINATION lib/static
  FILE_SET HEADERS
    COMPONENT Development
)

if(MSVC)
  set(_targets_to_install_pdb_for zrythm plugin-scanner)
  foreach(_target ${_targets_to_install_pdb_for})
    install(
      FILES $<TARGET_PDB_FILE:${_target}>
      COMPONENT Runtime
      DESTINATION ${CMAKE_INSTALL_BINDIR}
      CONFIGURATIONS Debug RelWithDebInfo
    )
  endforeach()
endif()
