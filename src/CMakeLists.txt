# SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_library(zrythm_lib STATIC)

target_sources(zrythm_lib
  PRIVATE
    globals.cpp
    project.cpp
    zrythm.cpp
    zrythm_app.cpp
    project/project_init_flow_manager.cpp
)

if(APPLE)
  list(APPEND zrythm_srcs Wrapper.m)
  target_sources(zrythm_lib
    PRIVATE
      Wrapper.m
  )
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../inc/utils/localization.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/../inc/utils/localization.h"
  @ONLY)
target_sources(zrythm_lib
  PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/../inc/utils/localization.h")

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
    "${GRESOURCE_COMPILED_FILE}"
    ${CMAKE_CURRENT_BINARY_DIR}/translators.h)

add_subdirectory(actions)
add_subdirectory(dsp)
add_subdirectory(gui)
add_subdirectory(io)
add_subdirectory(plugins)
if(ZRYTHM_YAML_PROJECT_COMPATIBILITY)
  add_subdirectory(schemas)
endif()
add_subdirectory(settings)
add_subdirectory(utils)

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

set_target_properties(zrythm_lib PROPERTIES OUTPUT_NAME zrythm_lib)
target_include_directories(zrythm_lib PUBLIC ${zrythm_include_dirs})
target_compile_definitions(zrythm_lib PUBLIC ${zrythm_definitions} ${gnome_compile_definitions})
target_compile_options(zrythm_lib PUBLIC ${zrythm_compile_args})
target_link_libraries(zrythm_lib PUBLIC ${zrythm_link_libs})

# precompile header
set(zrythm_pch ${CMAKE_CURRENT_SOURCE_DIR}/pch/zrythm_pch.h)
target_precompile_headers(zrythm_lib 
  PRIVATE 
    $<$<COMPILE_LANGUAGE:CXX>:${zrythm_pch}>)

add_executable(zrythm)
target_sources(zrythm
  PRIVATE main.cpp ${zrythm_resources})
target_link_libraries(zrythm PRIVATE zrythm_lib)
target_link_options(zrythm PRIVATE ${zrythm_link_args})
add_dependencies(zrythm
  zrythm-gresources desktop-file appdata-xml-merged)

if(WIN32)
  if(MSVC)
    # note: illegal to distribute MSVC runtime DLLs or build with static runtime libs
    set_target_properties(zrythm PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
  endif()
  set_target_properties(zrythm PROPERTIES WIN32_EXECUTABLE TRUE LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif()

install(TARGETS zrythm RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Run stoat
if(STOAT_FOUND AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_custom_target(run_stoat
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/run_stoat.sh
    DEPENDS zrythm_lib
  )
endif()
