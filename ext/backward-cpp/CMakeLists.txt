# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

set(backward_cpp_include_dirs ${CMAKE_CURRENT_SOURCE_DIR})
set(backward_cpp_compile_defs)
set(backward_cpp_libs Threads::Threads)

if(${OS_GNU})
  if(ZRYTHM_WITH_LIBDW)
    pkg_check_modules(LIBDW REQUIRED IMPORTED_TARGET libdw)
    list(APPEND backward_cpp_libs PkgConfig::LIBDW)
    list(APPEND backward_cpp_compile_defs
      BACKWARD_HAS_DW=1
      BACKWARD_HAS_UNWIND=1)
  endif()
elseif(APPLE)
  list(APPEND backward_cpp_compile_defs BACKWARD_HAS_UNWIND=1)
elseif(WIN32)
  list(APPEND backward_cpp_libs ${DBGHELP_LIB})
endif()

add_library(backward_cpp INTERFACE)
target_include_directories(backward_cpp SYSTEM INTERFACE ${backward_cpp_include_dirs})
target_link_libraries(backward_cpp INTERFACE ${backward_cpp_libs})
target_compile_definitions(backward_cpp INTERFACE ${backward_cpp_compile_defs})
add_library(backward_cpp::backward_cpp ALIAS backward_cpp)
