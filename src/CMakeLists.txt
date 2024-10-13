# SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_lib STATIC)

add_subdirectory(gui)

target_sources(zrythm_lib PRIVATE globals.cpp)

# Add the translators C string header to the sources
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/translators.h
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen-translators-list.py
          ${CMAKE_CURRENT_BINARY_DIR}/translators.h about
          ${CMAKE_SOURCE_DIR}/TRANSLATORS
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/gen-translators-list.py
          ${CMAKE_SOURCE_DIR}/TRANSLATORS
)
target_sources(zrythm_lib
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/translators.h)

add_subdirectory(common)

list(APPEND zrythm_link_libs
  midilib::midilib
  kissfft::kissfft
  whereami::whereami
  zrythm_optimized_audio_lib
  optimized_utils_lib
  vamp_plugins_lib
)

if(USE_WEAK_JACK)
  list(APPEND zrythm_link_libs weakjack::weakjack)
endif()

if(WIN32)
  # There are problems if added to zrythm_srcs directly
  target_sources(zrythm_lib PRIVATE ${win_res})
endif()

set_target_properties(zrythm_lib PROPERTIES
  OUTPUT_NAME zrythm_lib
)
target_include_directories(zrythm_lib PUBLIC ${zrythm_include_dirs})
target_compile_definitions(zrythm_lib PUBLIC ${zrythm_definitions} ${gnome_compile_definitions})
target_compile_options(zrythm_lib PUBLIC ${zrythm_compile_args})
target_link_libraries(zrythm_lib PUBLIC ${zrythm_link_libs})

# precompile header
set(zrythm_pch ${CMAKE_CURRENT_SOURCE_DIR}/pch/zrythm_pch.h)
target_precompile_headers(zrythm_lib
  PRIVATE 
    $<$<COMPILE_LANGUAGE:CXX>:${zrythm_pch}>)

target_sources(zrythm PRIVATE main.cpp)
target_link_libraries(zrythm PRIVATE zrythm_lib)
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

install(TARGETS zrythm RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(engine)
add_dependencies(zrythm zrythm-engine plugin-scanner)

# Run stoat
if(STOAT_FOUND AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_custom_target(run_stoat
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/run_stoat.sh
    DEPENDS zrythm_lib
  )
endif()
