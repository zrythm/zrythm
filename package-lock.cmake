# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: CC0-1.0
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
    "JSON_ImplicitConversions OFF"
    "JSON_SystemInclude ON"
)
# clap
CPMDeclarePackage(clap
  NAME clap
  VERSION 1.2.6
  GIT_TAG 69a69252fdd6ac1d06e246d9a04c0a89d9607a17
  GITHUB_REPOSITORY free-audio/clap
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)
# clap-helpers
CPMDeclarePackage(clap-helpers
  NAME clap-helpers
  VERSION 0.2
  GIT_TAG 3625679b5b32c2032df4134cae68cae0faa2ec36
  GITHUB_REPOSITORY free-audio/clap-helpers
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)
# juce
CPMDeclarePackage(juce
  NAME juce
  VERSION 8.0.10
  GIT_TAG 3af3ce009f6a02f6fa651008fffb5b41743a9fab
  GITHUB_REPOSITORY juce-framework/JUCE
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  SBOM_LICENSE_CONCLUDED "LicenseRef-JUCE-Commercial OR AGPL-3.0-only"
)
# MbedTLS
CPMDeclarePackage(MbedTLS
  NAME MbedTLS
  VERSION 3.6.3
  GIT_TAG 22098d41c6620ce07cf8a0134d37302355e1e5ef
  GITHUB_REPOSITORY Mbed-TLS/mbedtls
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "ENABLE_TESTING OFF"
    "ENABLE_PROGRAMS OFF"
    "DISABLE_PACKAGE_CONFIG_AND_INSTALL OFF"
    "USE_SHARED_MBEDTLS_LIBRARY OFF"
    "USE_STATIC_MBEDTLS_LIBRARY ON"
    "MBEDTLS_FATAL_WARNINGS OFF"
    "CMAKE_BUILD_TYPE Release"
)
# curl
CPMDeclarePackage(curl
  NAME curl
  VERSION 8.13.0
  GIT_TAG 1c3149881769e7bd79b072e48374e4c2b3678b2f
  GITHUB_REPOSITORY curl/curl
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "HTTP_ONLY ON"
    "BUILD_LIBCURL_DOCS OFF"
    "BUILD_MIS_DOCS OFF"
    "BUILD_TESTING OFF"
    "ENABLE_CURL_MANUAL OFF"
    "CURL_USE_MBEDTLS ON"
    "CURL_USE_PKGCONFIG OFF"
    "CURL_USE_LIBPSL OFF"
    "CURL_USE_LIBSSH2 OFF"
    # TODO: eventually enable https://github.com/nghttp2/nghttp2 to support HTTP/2
    "USE_NGHTTP2 OFF"
    "USE_LIBIDN2 OFF"
    "CURL_BROTLI OFF"
    "CURL_ZLIB OFF"
    "CURL_ZSTD OFF"
    "BUILD_CURL_EXE OFF"
    "BUILD_SHARED_LIBS OFF"
    "CMAKE_BUILD_TYPE Release"
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
  VERSION 11.2.0
  GIT_TAG 40626af88bd7df9a5fb80be7b25ac85b122d6c21
  GITHUB_REPOSITORY fmtlib/fmt
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "FMT_INSTALL OFF"
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
  VERSION 1.15.3
  GIT_TAG 6fa36017cfd5731d617e1a934f0e5ea9c4445b13
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
  VERSION 1.9.2
  GIT_TAG afa23b7699c17f1e26c88cbf95257b20d78d6247
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
# cpp_httplib
CPMDeclarePackage(cpp_httplib
  NAME cpp_httplib
  VERSION 0.20.1
  GIT_TAG 3af7f2c16147f3fbc6e4d717032daf505dc1652c
  GITHUB_REPOSITORY yhirose/cpp-httplib
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF"
    "HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF"
    "HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF"
    "HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF"
    "HTTPLIB_INSTALL OFF"
  SBOM_SKIP YES
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
  VERSION 0.5.0
  GIT_TAG 4bcf0751aa235c1d50c240f71fbde616693dec87
  GITHUB_REPOSITORY aurora-opensource/au
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "AU_EXCLUDE_GTEST_DEPENDENCY ON"
)
# Boost
CPMDeclarePackage(Boost
  NAME Boost
  VERSION 1.88.0
  GIT_TAG 199ef13d6034c85232431130142159af3adfce22
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
    libs/static_assert
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
