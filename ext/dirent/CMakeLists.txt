# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if(MSVC)
  add_library(dirent INTERFACE)
  target_include_directories(dirent INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
  set_target_properties(dirent PROPERTIES
    VERSION 0.1.0
  )
endif()
