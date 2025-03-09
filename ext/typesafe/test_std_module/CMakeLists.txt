# Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

add_executable(type_safe_test_std_module test.cpp)
target_link_libraries(type_safe_test_std_module PUBLIC type_safe)
target_include_directories(type_safe_test_std_module PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Hopefully MS will provide a better/automatic/IJW way to use the standard library module, in other words "import std;" should just work
if( MSVC )
    # Try to find "VCToolsInstallDir" from the environment variable, then look for "std.ixx" according to its path
    if (DEFINED ENV{VCToolsInstallDir} AND EXISTS $ENV{VCToolsInstallDir} AND EXISTS $ENV{VCToolsInstallDir}modules/std.ixx)
        target_sources(type_safe PUBLIC "$ENV{VCToolsInstallDir}modules/std.ixx")
    else ()
        # We use a path relative to the compiler location which should be stable and avoid VS upgrade/version/preview install location issues
        cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH  CompilerDir)
        target_sources(type_safe PUBLIC "${CompilerDir}/../../../modules/std.ixx")
    endif (DEFINED ENV{VCToolsInstallDir} AND EXISTS $ENV{VCToolsInstallDir} AND EXISTS $ENV{VCToolsInstallDir}modules/std.ixx)
endif()
