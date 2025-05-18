# SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if(WIN32)
  # Enable processing of resource files
  enable_language(RC)

  # Set the resource compiler for Windows
  if(MSVC)
    set(CMAKE_RC_COMPILER rc)
  elseif(MINGW)
    set(CMAKE_RC_COMPILER windres)
  endif()

  set(ZRYTHM_MAJOR_VER 1)
  set(ZRYTHM_MINOR_VER 0)
  set(ZRYTHM_PATCH_VER 0)
  set(ZRYTHM_MICRO_VER 0)
  set(PRODUCT_VERSION ${PROJECT_VERSION})

  configure_file(
    zrythm.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/zrythm.rc
    @ONLY)

  configure_file(
    zrythm.ico
    ${CMAKE_CURRENT_BINARY_DIR}/zrythm.ico
    COPYONLY)

  set(windows_rc_file ${CMAKE_CURRENT_BINARY_DIR}/zrythm.rc)
  set(windows_icon_file ${CMAKE_CURRENT_BINARY_DIR}/zrythm.ico)

  # Set up resource compilation
  set_source_files_properties(
    ${windows_rc_file}
    PROPERTIES
    VS_TOOL_OVERRIDE "Resource Compiler"
  )
endif()
