# Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

set(TYPE_SAFE_STANDARDESE_DOWNLOAD_DIRECTORY "" CACHE PATH "path to directory with pre-built standardese")

find_package(standardese QUIET)
if(NOT standardese_FOUND)
    find_program(STANDARDESE_TOOL standardese ${TYPE_SAFE_STANDARDESE_DOWNLOAD_DIRECTORY})
    include(${TYPE_SAFE_STANDARDESE_DOWNLOAD_DIRECTORY}/standardese-config.cmake)
endif()

standardese_generate(type_safe
                     CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/standardese.config
                     INCLUDE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/
                                       ${CMAKE_CURRENT_SOURCE_DIR}/../external/debug_assert/
                     INPUT ${header_files})
