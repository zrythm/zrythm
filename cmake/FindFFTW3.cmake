# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

find_package(PkgConfig REQUIRED)

option(ZRYTHM_FFTW3_THREADS_SEPARATE "Whether to look for fftw3_threads pkgconfig files (or libs directly if not found) separate from the main fftw3 one" ON)
# Method to look for separate fftw3_threads. Auto: look for dependency first and then library if not found.
set(ZRYTHM_FFTW3_THREADS_SEPARATE_TYPE "auto" CACHE STRING "Method to look for separate fftw3_threads")
set_property(CACHE ZRYTHM_FFTW3_THREADS_SEPARATE_TYPE PROPERTY STRINGS auto dependency library)
option(ZRYTHM_FFTW3F_SEPARATE "Whether to look for fftw3f separate of fftw3" ON)

# FFTW3 - this is a mess because every distro packages fftw3
# in various different ways

set(FFTW3_DEPS)

pkg_check_modules(FFTW3 REQUIRED IMPORTED_TARGET fftw3>=3.3.5)
list(APPEND FFTW3_DEPS PkgConfig::FFTW3)

if(ZRYTHM_IS_FLATPAK)
  # For some reason this doesn't work in GNOME builder with flatpak below when
  # dirs: and static: are passed so handle it separately here...
  find_library(FFTW3_THREADS fftw3_threads REQUIRED)
  find_library(FFTW3F_THREADS fftw3f_threads REQUIRED)
  list(APPEND FFTW3_DEPS ${FFTW3_THREADS} ${FFTW3F_THREADS})
elseif(ZRYTHM_FFTW3_THREADS_SEPARATE)
  set(SEARCH_PATHS)
  if(NOT WIN32)
    set(SEARCH_PATHS /usr/lib /usr/local/lib /opt/homebrew/lib)
  endif()

  if(ZRYTHM_FFTW3_THREADS_SEPARATE_TYPE STREQUAL "auto")
    pkg_check_modules(FFTW3_THREADS fftw3_threads)
    pkg_check_modules(FFTW3F_THREADS fftw3f_threads)
    if(FFTW3_THREADS_FOUND AND FFTW3F_THREADS_FOUND)
      list(APPEND FFTW3_DEPS PkgConfig::FFTW3_THREADS PkgConfig::FFTW3F_THREADS)
    else()
      find_library(FFTW3_THREADS fftw3_threads REQUIRED PATHS ${SEARCH_PATHS})
      find_library(FFTW3F_THREADS fftw3f_threads REQUIRED PATHS ${SEARCH_PATHS})
      list(APPEND FFTW3_DEPS ${FFTW3_THREADS} ${FFTW3F_THREADS})
    endif()
  elseif(ZRYTHM_FFTW3_THREADS_SEPARATE_TYPE STREQUAL "dependency")
    pkg_check_modules(FFTW3_THREADS REQUIRED IMPORTED_TARGET fftw3_threads)
    pkg_check_modules(FFTW3F_THREADS REQUIRED IMPORTED_TARGET fftw3f_threads)
    list(APPEND FFTW3_DEPS PkgConfig::FFTW3_THREADS PkgConfig::FFTW3F_THREADS)
  elseif(ZRYTHM_FFTW3_THREADS_SEPARATE_TYPE STREQUAL "library")
    find_library(FFTW3_THREADS fftw3_threads REQUIRED PATHS ${SEARCH_PATHS})
    find_library(FFTW3F_THREADS fftw3f_threads REQUIRED PATHS ${SEARCH_PATHS})
    list(APPEND FFTW3_DEPS ${FFTW3_THREADS} ${FFTW3F_THREADS})
  else()
    message(FATAL_ERROR "ZRYTHM_FFTW3_THREADS_SEPARATE_TYPE must be either of: library, dependency, auto")
  endif()
endif()

if(WIN32 OR ZRYTHM_FFTW3F_SEPARATE)
  # msys2 provides a separate fftw3f entry in pkg-config
  pkg_check_modules(FFTW3F REQUIRED IMPORTED_TARGET fftw3f)
  list(APPEND FFTW3_DEPS PkgConfig::FFTW3F)
endif()

set(FFTW3_FUNCS
  fftw_make_planner_thread_safe
  fftwf_make_planner_thread_safe
)

if(0) # TODO
foreach(func ${FFTW3_FUNCS})
  include(CheckSymbolExists)
  check_symbol_exists(${func} "fftw3.h" HAVE_${func})
  if(NOT HAVE_${func})
    message(WARNING "${func} missing. If linking fails, on some systems you may have to "
                    "point Zrythm to the library that provides it directly, eg., "
                    "LDFLAGS=\"$LDFLAGS /usr/lib/libfftw3_threads.so "
                    "/usr/lib/libfftw3f_threads.so\"")
  endif()
endforeach()
endif()

# Create an interface library for FFTW3
add_library(FFTW3::FFTW3 INTERFACE IMPORTED)
set_target_properties(FFTW3::FFTW3 PROPERTIES
  INTERFACE_LINK_LIBRARIES "${FFTW3_DEPS}"
)