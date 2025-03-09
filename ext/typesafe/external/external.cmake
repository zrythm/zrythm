# Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

find_package(debug_assert QUIET)
if(debug_assert_FOUND)
    set(TYPE_SAFE_HAS_IMPORTED_TARGETS ON)
else()
    set(TYPE_SAFE_HAS_IMPORTED_TARGETS OFF)
    if(TARGET debug_assert)
        message(STATUS "Using inherited debug_assert target")
    else()
        message(STATUS "Using vendored debug_assert")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/debug_assert EXCLUDE_FROM_ALL)
    endif()
endif()
