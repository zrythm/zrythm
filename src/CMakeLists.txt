# SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_gui_lib STATIC)

# each executable/library has its own subdirectory
add_subdirectory(plugin-scanner)
add_subdirectory(utils)
# add_subdirectory(lv2apply)
add_subdirectory(dsp)
add_subdirectory(gui)

target_sources(zrythm_gui_lib PRIVATE globals.cpp)

if(0) # TODO
  # Add the translators C string header to the sources
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/translators.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen-translators-list.py
            ${CMAKE_CURRENT_BINARY_DIR}/translators.h about
            ${CMAKE_SOURCE_DIR}/TRANSLATORS
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/gen-translators-list.py
            ${CMAKE_SOURCE_DIR}/TRANSLATORS
  )
  target_sources(zrythm_gui_lib
    PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}/translators.h)
endif()

list(APPEND zrythm_link_libs
  midilib::midilib
  kissfft::kissfft
)

if(WIN32)
  # There are problems if added to zrythm_srcs directly
  target_sources(zrythm_gui_lib PRIVATE ${win_res})
endif()

set_target_properties(zrythm_gui_lib PROPERTIES
  OUTPUT_NAME zrythm_gui_lib
)
target_link_libraries(zrythm_gui_lib
  PUBLIC
    ${zrythm_link_libs}
    zrythm::dsp_lib
    zrythm::include_dirs
    zrythm::all_compile_options
)
target_precompile_headers(zrythm_gui_lib REUSE_FROM zrythm::dsp_lib)

target_sources(zrythm PRIVATE main.cpp)
target_link_libraries(zrythm PRIVATE zrythm_gui_lib)
target_link_options(zrythm PRIVATE ${zrythm_link_args})
add_dependencies(zrythm appdata-xml-merged)

if(OS_GNU)
  add_dependencies(zrythm desktop-file)
endif()

if(WIN32)
  set_target_properties(zrythm PROPERTIES
    WIN32_EXECUTABLE TRUE
    VS_DEBUGGER_ENVIRONMENT "GSETTINGS_SCHEMA_DIR=${CMAKE_INSTALL_FULL_DATADIR}/glib-2.0/schemas;G_DEBUG=fatal_criticals"
  )
  if(MSVC)
    target_link_options(zrythm PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
  endif()
  target_sources(zrythm PRIVATE ${windows_rc_file} ${windows_icon_file})
endif()

set_target_properties(zrythm PROPERTIES
  VS_DEBUGGER_ENVIRONMENT "GSETTINGS_SCHEMA_DIR=${CMAKE_INSTALL_FULL_DATADIR}/glib-2.0/schemas;G_DEBUG=fatal_criticals"
  XCODE_SCHEME_ENVIRONMENT "GSETTINGS_SCHEMA_DIR=${CMAKE_INSTALL_FULL_DATADIR}/glib-2.0/schemas;G_DEBUG=fatal_criticals"
  MACOSX_BUNDLE_GUI_IDENTIFIER "org.zrythm.Zrythm"
  MACOSX_BUNDLE_BUNDLE_VERSION ${ZRYTHM_VERSION_STRING_FULL}
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${ZRYTHM_VERSION}
  MACOSX_BUNDLE TRUE
  WIN32_EXECUTABLE TRUE
)

install(TARGETS zrythm
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(engine)
add_dependencies(zrythm zrythm-engine plugin-scanner)
