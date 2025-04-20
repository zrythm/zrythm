# Copyright (C) 2016-2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.10)
project(DEBUG_ASSERT)

# Determine if debug_assert is built as a subproject (using add_subdirectory)
# or if it is the master project.
set(MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MASTER_PROJECT ON)
endif ()

# options
option(DEBUG_ASSERT_NO_STDIO "whether or not the default_handler uses fprintf() to print an error message" OFF)
if(DEBUG_ASSERT_NO_STDIO)
    set(_debug_assert_no_stdio DEBUG_ASSERT_NO_STDIO)
endif()

option(DEBUG_ASSERT_DISABLE "completely disable assertion macro" OFF)
if(DEBUG_ASSERT_DISABLE)
    set(_debug_assert_disable DEBUG_ASSERT_DISABLE)
endif()

set(DEBUG_ASSERT_MARK_UNREACHABLE "" CACHE STRING "override of DEBUG_ASSERT_MARK_UNREACHABLE")
if(DEBUG_ASSERT_MARK_UNREACHABLE)
    set(_debug_assert_mark_unreachable "DEBUG_ASSERT_MARK_UNREACHABLE=${DEBUG_ASSERT_MARK_UNREACHABLE}")
endif()

set(DEBUG_ASSERT_ASSUME "" CACHE STRING "override of DEBUG_ASSERT_ASSUME macro")
if(DEBUG_ASSERT_ASSUME)
    set(_debug_assert_assume "DEBUG_ASSERT_ASSUME=${DEBUG_ASSERT_ASSUME}")
endif()

set(DEBUG_ASSERT_FORCE_INLINE "" CACHE STRING "override of DEBUG_ASSERT_FORCE_INLINE macro")
if(DEBUG_ASSERT_FORCE_INLINE)
    set(_debug_assert_force_inline "DEBUG_ASSERT_FORCE_INLINE=${DEBUG_ASSERT_FORCE_INLINE}")
endif()

option(DEBUG_ASSERT_INSTALL "Generate the install target." ${MASTER_PROJECT})

# interface target
add_library(debug_assert INTERFACE)
target_sources(debug_assert INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/debug_assert.hpp>)
target_include_directories(debug_assert INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_include_directories(debug_assert SYSTEM INTERFACE $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)
target_compile_definitions(debug_assert INTERFACE
                                        ${_debug_assert_no_stdio}
                                        ${_debug_assert_disable}
                                        ${_debug_assert_mark_unreachable}
                                        ${_debug_assert_assume}
                                        ${_debug_assert_force_inline})

# example target
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    add_executable(debug_assert_example EXCLUDE_FROM_ALL example.cpp)
    target_link_libraries(debug_assert_example PUBLIC debug_assert)
endif()

# Create package config files
include( CMakePackageConfigHelpers )
set(CONFIG_PACKAGE_INSTALL_DIR lib/cmake/debug_assert)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/debug_assert-config.cmake "
include(\${CMAKE_CURRENT_LIST_DIR}/debug_assert-targets.cmake)
set(debug_assert_LIBRARY debug_assert)
set(debug_assert_LIBRARIES debug_assert)
")

set(DEBUG_ASSERT_CMAKE_SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P})
set(CMAKE_SIZEOF_VOID_P "")
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/debug_assert-config-version.cmake
  VERSION 1.3.1
  COMPATIBILITY SameMajorVersion
)
set(CMAKE_SIZEOF_VOID_P ${DEBUG_ASSERT_CMAKE_SIZEOF_VOID_P})

if(DEBUG_ASSERT_INSTALL)
    # Install target and header
    install(TARGETS debug_assert
        EXPORT debug_assert-targets
        DESTINATION lib)

    install(FILES debug_assert.hpp DESTINATION include)

    install( EXPORT debug_assert-targets
    DESTINATION
        ${CONFIG_PACKAGE_INSTALL_DIR}
    )

    install( FILES
    ${CMAKE_CURRENT_BINARY_DIR}/debug_assert-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/debug_assert-config-version.cmake
    DESTINATION
        ${CONFIG_PACKAGE_INSTALL_DIR} )
endif()

