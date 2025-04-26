# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# Currently we are using bundled soxr due to a patch to fix CMakeLists.txt
# being needed but this is kept for reference in case we ever want to link
# against system soxr

find_package(soxr QUIET)
if(NOT soxr_FOUND)
  pkg_check_modules(SOXR soxr IMPORTED_TARGET)
  if(SOXR_FOUND)
    add_library(soxr::soxr ALIAS PkgConfig::SOXR)
  else()
    find_library(SOXR_LIB soxr
      PATHS ${SOXR_LIB_SEARCH_DIR}
      REQUIRED
    )
    find_path(SOXR_INCLUDE_DIR soxr.h
      PATHS ${SOXR_INCLUDE_DIR_SEARCH_DIR}
      REQUIRED
    )
    add_library(soxr::soxr UNKNOWN IMPORTED)
    set_target_properties(soxr::soxr PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${SOXR_INCLUDE_DIR}"
      IMPORTED_LOCATION "${SOXR_LIB}"
    )
  endif()
endif()