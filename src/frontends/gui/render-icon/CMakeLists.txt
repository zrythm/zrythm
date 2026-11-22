# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# Renders the window icon SVG to PNG files at build time (see main.cpp)
add_executable(render-icon main.cpp)
target_link_libraries(render-icon PRIVATE zrythm::qt_libs)
set_target_properties(render-icon PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)

if(WIN32)
  # The PNG generation runs as soon as this tool links, before the zrythm
  # target deploys the Qt DLLs into the binary output directory, so this
  # tool needs its own deployment to run from the build tree.
  add_custom_command(TARGET render-icon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:render-icon>
      $<TARGET_RUNTIME_DLLS:render-icon>
    COMMAND "${WINDEPLOYQT_EXECUTABLE}"
      --pdb --force --no-compiler-runtime
      $<TARGET_FILE:render-icon>
    COMMAND_EXPAND_LISTS
  )
endif()
