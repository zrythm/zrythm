# SPDX-FileCopyrightText: © 2017, University of Cincinnati
# SPDX-License-Identifier: BSD-3-Clause
# Based on https://crascit.com/2016/04/09/using-ccache-with-cmake/

cmake_minimum_required(VERSION 3.10)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    # Set up wrapper scripts
    set(C_LAUNCHER   "${CCACHE_PROGRAM}")
    set(CXX_LAUNCHER "${CCACHE_PROGRAM}")
    set(CUDA_LAUNCHER "${CCACHE_PROGRAM}")

    file(WRITE "${CMAKE_BINARY_DIR}/launch-c" ""
        "#!/usr/bin/env sh\n"
        "\n"
        "# Xcode generator doesn't include the compiler as the\n"
        "# first argument, Ninja and Makefiles do. Handle both cases.\n"
        "if [ \"$1\" = \"${CMAKE_C_COMPILER}\" ] ; then\n"
        "    shift\n"
        "fi\n"
        "\n"
        "export CCACHE_CPP2=true\n"
        "exec \"${C_LAUNCHER}\" \"${CMAKE_C_COMPILER}\" \"$@\"\n"
        )

    file(WRITE "${CMAKE_BINARY_DIR}/launch-cxx" ""
        "#!/usr/bin/env sh\n"
        "\n"
        "# Xcode generator doesn't include the compiler as the\n"
        "# first argument, Ninja and Makefiles do. Handle both cases.\n"
        "if [ \"$1\" = \"${CMAKE_CXX_COMPILER}\" ] ; then\n"
        "    shift\n"
        "fi\n"
        "\n"
        "export CCACHE_CPP2=true\n"
        "exec \"${CXX_LAUNCHER}\" \"${CMAKE_CXX_COMPILER}\" \"$@\"\n"
        )

    # Cuda support only added in CMake 3.10
    file(WRITE "${CMAKE_BINARY_DIR}/launch-cuda" ""
        "#!/usr/bin/env sh\n"
        "\n"
        "# Xcode generator doesn't include the compiler as the\n"
        "# first argument, Ninja and Makefiles do. Handle both cases.\n"
        "if [ \"$1\" = \"${CMAKE_CUDA_COMPILER}\" ] ; then\n"
        "    shift\n"
        "fi\n"
        "\n"
        "export CCACHE_CPP2=true\n"
        "exec \"${CUDA_LAUNCHER}\" \"${CMAKE_CUDA_COMPILER}\" \"$@\"\n"
        )


    execute_process(COMMAND chmod a+rx
                     "${CMAKE_BINARY_DIR}/launch-c"
                     "${CMAKE_BINARY_DIR}/launch-cxx"
                     "${CMAKE_BINARY_DIR}/launch-cuda"
    )

    if(CMAKE_GENERATOR STREQUAL "Xcode")
        # Set Xcode project attributes to route compilation and linking
        # through our scripts
        set(CMAKE_XCODE_ATTRIBUTE_CC         "${CMAKE_BINARY_DIR}/launch-c")
        set(CMAKE_XCODE_ATTRIBUTE_CXX        "${CMAKE_BINARY_DIR}/launch-cxx")
        set(CMAKE_XCODE_ATTRIBUTE_LD         "${CMAKE_BINARY_DIR}/launch-c")
        set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS "${CMAKE_BINARY_DIR}/launch-cxx")
    else()
        # Support Unix Makefiles and Ninja
        set(CMAKE_C_COMPILER_LAUNCHER   "${CMAKE_BINARY_DIR}/launch-c")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_BINARY_DIR}/launch-cxx")
        set(CMAKE_CUDA_COMPILER_LAUNCHER "${CMAKE_BINARY_DIR}/launch-cuda")
    endif()

    message(STATUS "Using CCache")
else()
    message(STATUS "Not using CCache")
endif()
