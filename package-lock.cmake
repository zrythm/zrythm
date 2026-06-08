# SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# CPM Package Lock
# This file should be committed to version control

# nlohmann_json
CPMDeclarePackage(nlohmann_json
  NAME nlohmann_json
  VERSION 3.12.1
  # 3.12.0 + some commits later
  GIT_TAG 230bfd15a2bb7f01ebb3fcd3cf898b697ef43c48
  GITHUB_REPOSITORY nlohmann/json
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    # The validator requires explicit conversions to be enabled
    # See https://github.com/pboettch/json-schema-validator/issues/372
    "JSON_ImplicitConversions ON"
    "JSON_SystemInclude ON"
)
# clap
CPMDeclarePackage(clap
  NAME clap
  VERSION 1.2.7
  GIT_TAG 29ffcc273be7c7c651f6c9953b99e69700e2387a
  GITHUB_REPOSITORY free-audio/clap
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)
# clap-helpers
CPMDeclarePackage(clap-helpers
  NAME clap-helpers
  VERSION 0.3
  GIT_TAG d0ddaf0b581e083a1f615d6509f1af907a5ce82c
  GITHUB_REPOSITORY alex-tee/clap-helpers
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)
# clap-juce-extensions
CPMDeclarePackage(clap-juce-extensions
  NAME clap-juce-extensions
  VERSION 0.1
  GIT_TAG 36c0a14ab03918d4258eaba2ea40a3fadb992da6
  GITHUB_REPOSITORY alex-tee/clap-juce-extensions
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)
# juce
CPMDeclarePackage(juce
  NAME juce
  VERSION 8.0.13
  GIT_TAG 7c9d3783b127263d72bb65fe0a7e2dc8a02a7ac2
  GITHUB_REPOSITORY juce-framework/JUCE
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  SBOM_LICENSE_CONCLUDED "LicenseRef-JUCE-Commercial OR AGPL-3.0-only"
)
# SndFile
CPMDeclarePackage(SndFile
  NAME SndFile
  VERSION 1.2.2
  GIT_TAG 72f6af15e8f85157bd622ed45b979025828b7001
  GITHUB_REPOSITORY libsndfile/libsndfile
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "BUILD_EXAMPLES OFF"
    "BUILD_TESTING OFF"
    "ENABLE_EXTERNAL_LIBS ON"
    "ENABLE_CPACK OFF"
    "ENABLE_PACKAGE_CONFIG OFF"
    "INSTALL_PKGCONFIG_MODULE OFF"
    "INSTALL_MANPAGES OFF"
  SBOM_SKIP YES
)
# zstd
CPMDeclarePackage(zstd
  NAME zstd
  VERSION 1.5.7
  GIT_TAG f8745da6ff1ad1e7bab384bd1f9d742439278e99
  GITHUB_REPOSITORY facebook/zstd
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  SOURCE_SUBDIR build/cmake
  OPTIONS
    "ZSTD_BUILD_STATIC ON"
    "ZSTD_BUILD_SHARED OFF"
    "ZSTD_LEGACY_SUPPORT OFF"
    "ZSTD_BUILD_PROGRAMS OFF"
    "ZSTD_BUILD_CONTRIB OFF"
    "ZSTD_BUILD_TESTS OFF"
)
# xxHash
CPMDeclarePackage(xxHash
  NAME xxHash
  VERSION 0.8.2
  GIT_TAG bbb27a5efb85b92a0486cf361a8635715a53f6ba
  GITHUB_REPOSITORY Cyan4973/xxHash
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  SOURCE_SUBDIR cmake_unofficial
  OPTIONS
    "XXHASH_BUILD_XXHSUM OFF"
)
# magic_enum
CPMDeclarePackage(magic_enum
  NAME magic_enum
  VERSION 0.9.6
  GIT_TAG dd6a39d0ba1852cf06907e0f0573a2a10d23c2ad
  GITHUB_REPOSITORY Neargye/magic_enum
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)
# fmt
CPMDeclarePackage(fmt
  NAME fmt
  VERSION 12.1.0
  GIT_TAG 407c905e45ad75fc29bf0f9bb7c5c2fd3475976f
  GITHUB_REPOSITORY fmtlib/fmt
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "FMT_INSTALL OFF"
    "FMT_SYSTEM_HEADERS ON"
)
# fast_float
CPMDeclarePackage(fast_float
  NAME fast_float
  VERSION 6.1.6
  GIT_TAG 00c8c7b0d5c722d2212568d915a39ea73b08b973
  GITHUB_REPOSITORY fastfloat/fast_float
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "FASTFLOAT_CXX_STANDARD 17"
    "FASTFLOAT_TEST OFF"
    "FASTFLOAT_INSTALL OFF"
)
# scnlib
CPMDeclarePackage(scnlib
  NAME scnlib
  VERSION 4.0.1
  GIT_TAG e937be1a52588621b406d58ce8614f96bb5de747
  GITHUB_REPOSITORY eliaskosunen/scnlib
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "SCN_TESTS OFF"
    "SCN_DOCS OFF"
    "SCN_EXAMPLES OFF"
    "SCN_INSTALL OFF"
    "SCN_USE_EXTERNAL_FAST_FLOAT ON"
)
# spdlog
CPMDeclarePackage(spdlog
  NAME spdlog
  VERSION 1.17.0
  GIT_TAG 79524ddd08a4ec981b7fea76afd08ee05f83755d
  GITHUB_REPOSITORY gabime/spdlog
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "SPDLOG_INSTALL OFF"
)
# type_safe
CPMDeclarePackage(type_safe
  NAME type_safe
  VERSION 0.2.4
  GIT_TAG 1dbea7936a1e389caa692d01bb3f5ba4b6da7d82
  GITHUB_REPOSITORY foonathan/type_safe
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)
# GTest
CPMDeclarePackage(GTest
  NAME GTest
  VERSION 1.16.0
  GIT_TAG 6910c9d9165801d8827d628cb72eb7ea9dd538c5
  GITHUB_REPOSITORY google/googletest
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "INSTALL_GTEST OFF"
  SBOM_SKIP YES
)
# benchmark
CPMDeclarePackage(benchmark
  NAME benchmark
  VERSION 1.9.5
  GIT_TAG 192ef10025eb2c4cdd392bc502f0c852196baa48
  GITHUB_REPOSITORY google/benchmark
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "BENCHMARK_ENABLE_TESTING OFF"
    "BENCHMARK_ENABLE_INSTALL OFF"
    "BENCHMARK_INSTALL_DOCS OFF"
    "CMAKE_BUILD_TYPE Release"
  SBOM_SKIP YES
)
# gsl-lite
CPMDeclarePackage(gsl-lite
  NAME gsl-lite
  VERSION 1.0.1
  GIT_TAG 56dab5ce071c4ca17d3e0dbbda9a94bd5a1cbca1
  GITHUB_REPOSITORY gsl-lite/gsl-lite
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)
# tbb
set(tbb_opts "")
list(APPEND tbb_opts "TBB_STRICT OFF")
list(APPEND tbb_opts "TBB_TEST OFF")
list(APPEND tbb_opts "TBB_INSTALL OFF")
list(APPEND tbb_opts "TBBMALLOC_BUILD OFF")
list(APPEND tbb_opts "TBBMALLOC_PROXY_BUILD OFF")
if(ZRYTHM_ENABLE_SANITIZER_ADDRESS)
  list(APPEND tbb_opts "TBB_SANITIZE address")
elseif(ZRYTHM_ENABLE_SANITIZER_THREAD)
  list(APPEND tbb_opts "TBB_SANITIZE thread")
elseif(ZRYTHM_ENABLE_SANITIZER_MEMORY)
  list(APPEND tbb_opts "TBB_SANITIZE memory")
endif()
CPMDeclarePackage(tbb
  NAME tbb
  GITHUB_REPOSITORY zrythm/oneTBB
  VERSION 2022.2.0
  GIT_TAG fe153f04aebb223d500f27bc3f93374a012273ec
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
  OPTIONS ${tbb_opts}
)
# farbot
CPMDeclarePackage(farbot
  NAME farbot
  VERSION 0.1
  GIT_TAG 2a2bd99eefada6c78dff04bc0692912366952a86
  GITHUB_REPOSITORY zrythm/farbot
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)
# mp-units
CPMDeclarePackage(mp-units
  NAME mp-units
  VERSION 2.4.9
  GIT_TAG 213ce0e6dd26e20287b585bcd567d3b25d42352d
  GITHUB_REPOSITORY alex-tee/mp-units
  SOURCE_SUBDIR src
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "MP_UNITS_BUILD_AS_SYSTEM_HEADERS ON"
    "MP_UNITS_API_STD_FORMAT OFF"
    "MP_UNITS_API_NO_CRTP ON"
    "MP_UNITS_API_FREESTANDING OFF"
    "MP_UNITS_API_CONTRACTS NONE"
    "MP_UNITS_BUILD_CXX_MODULES OFF"
    "MP_UNITS_API_NATURAL_UNITS OFF"
    "MP_UNITS_INSTALL OFF"
)
# au
CPMDeclarePackage(au
  NAME au
  VERSION 0.5.1
  GIT_TAG a4cd560f20d3dfb71334a47c4de0e7086f28c6a5
  GITHUB_REPOSITORY aurora-opensource/au
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "AU_EXCLUDE_GTEST_DEPENDENCY ON"
)
# Boost
CPMDeclarePackage(Boost
  NAME Boost
  VERSION 1.91.0
  GIT_TAG 1a80576db6b70828803819fb6925132193bc5d0e
  GITHUB_REPOSITORY boostorg/boost
  GIT_SHALLOW TRUE
  GIT_SUBMODULES
    libs/assert
    libs/bind
    libs/circular_buffer
    libs/concept_check
    libs/config
    libs/container
    libs/container_hash
    libs/core
    libs/describe
    libs/detail
    libs/fusion
    libs/function
    libs/functional
    libs/function_types
    libs/intrusive
    libs/io
    libs/integer
    libs/iterator
    libs/move
    libs/mp11
    libs/mpl
    libs/multi_index
    libs/optional
    libs/predef
    libs/preprocessor
    libs/smart_ptr
    libs/stl_interfaces
    libs/throw_exception
    libs/tuple
    libs/typeof
    libs/type_traits
    libs/unordered
    libs/utility
    tools/cmake
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "BOOST_INCLUDE_LIBRARIES
      circular_buffer
      container
      describe
      multi_index
      stl_interfaces
      unordered"
)
CPMDeclarePackage(nlohmann_json_schema_validator
  NAME nlohmann_json_schema_validator
  VERSION 2.4.0
  GIT_TAG c780404a84dd9ba978ba26bc58d17cb43fa7bc80
  GITHUB_REPOSITORY pboettch/json-schema-validator
  GIT_SHALLOW TRUE
)
# Tracy
CPMDeclarePackage(Tracy
  NAME Tracy
  VERSION 05cceee0df3b8d7c6fa87e9638af311dbabc63cb
  GIT_TAG v0.13.1
  GITHUB_REPOSITORY wolfpld/tracy
  GIT_SHALLOW TRUE
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "TRACY_ENABLE ON"
    "TRACY_STATIC ON"
    "TRACY_NO_BROADCAST ON"
    "TRACY_NO_CODE_TRANSFER ON"
    "TRACY_NO_CONTEXT_SWITCH ON"
    "TRACY_NO_FRAME_IMAGE ON"
    "TRACY_NO_SYSTEM_TRACING ON"
    "TRACY_NO_VSYNC_CAPTURE ON"
    "TRACY_DELAYED_INIT ON"
    "TRACY_MANUAL_LIFETIME ON"
    "TRACY_NO_CRASH_HANDLER ON"
)
