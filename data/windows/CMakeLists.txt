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

  set(RC_FILE ${CMAKE_CURRENT_BINARY_DIR}/zrythm.rc)
  set(ICON_FILE ${CMAKE_CURRENT_BINARY_DIR}/zrythm.ico)

  # Add the resource file to the target
  target_sources(${prog_name_lowercase} PRIVATE ${RC_FILE} ${ICON_FILE})

  # Set up resource compilation
  set_source_files_properties(
    ${RC_FILE}
    PROPERTIES
    VS_TOOL_OVERRIDE "Resource Compiler"
  )
endif()
