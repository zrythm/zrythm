# SPDX-FileCopyrightText: © 2019-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_subdirectory(backward-cpp)
add_subdirectory(crill)
add_subdirectory(dirent)
add_subdirectory(lv2)
add_subdirectory(moodycamel)

set(JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED ON)
set(JUCE_MODULES_ONLY ON)
if(OS_GNU)
  set(JUCE_NEEDS_CURL ON)
endif()
add_library(juce_lib STATIC)

CPMFindPackage(
  NAME juce
  GITHUB_REPOSITORY juce-framework/JUCE
  VERSION 8.0.8
  GIT_TAG d6181bde38d858c283c3b7bf699ce6340c050b5d
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
)

if(ZRYTHM_WITH_JUCE_CLAP_HOSTING)
  CPMFindPackage(
    NAME clap
    GITHUB_REPOSITORY free-audio/clap
    VERSION 1.2.2
    GIT_TAG 27f20f81dec40b930d79ef429fd35dcc2d45db5b
    EXCLUDE_FROM_ALL YES
    SYSTEM YES
  )
  CPMFindPackage(
    NAME clap-helpers
    GITHUB_REPOSITORY free-audio/clap-helpers
    # main branch
    GIT_TAG 59791394dc26637d9425c2745233a979602be2a7
    EXCLUDE_FROM_ALL YES
    SYSTEM YES
  )

  juce_add_module(juce_clap_hosting)
  add_library(juce_clap_hosting_lib STATIC)
  target_link_libraries(juce_clap_hosting_lib
    PRIVATE
      juce_clap_hosting
      juce_lib
      clap
      clap-helpers
    PUBLIC
      juce::juce_recommended_config_flags
  )
  if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
   target_link_libraries(juce_clap_hosting_lib
      PUBLIC juce::juce_recommended_lto_flags
    )
  endif()
  target_include_directories(juce_clap_hosting_lib
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/juce_clap_hosting"
  )
endif()

target_link_libraries(juce_lib
  PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
  PUBLIC
    juce::juce_recommended_config_flags
    # juce::juce_recommended_warning_flags
)
if(APPLE)
  find_library(AUDIO_UNIT_LIB CoreAudioKit REQUIRED)
  target_link_libraries(juce_lib INTERFACE ${AUDIO_UNIT_LIB})
endif()
if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  target_link_libraries(juce_lib
    PUBLIC juce::juce_recommended_lto_flags
  )
endif()
target_include_directories(juce_lib
  INTERFACE "${juce_SOURCE_DIR}/modules"
  # FIXME should be private
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/ladspa"
)

target_compile_definitions(juce_lib
  PUBLIC
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_WEB_BROWSER=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_LADSPA=1
    JUCE_PLUGINHOST_AU=1
    JUCE_DISABLE_JUCE_VERSION_PRINTING=1
)
if(ZRYTHM_WITH_JUCE_LV2_HOSTING)
  target_compile_definitions(juce_lib PUBLIC JUCE_PLUGINHOST_LV2=1)
endif()

target_compile_definitions(juce_lib
  PUBLIC
    $<IF:$<CONFIG:Debug>,JUCE_DEBUG=1;_DEBUG=1;DEBUG=1,JUCE_DEBUG=0;_NDEBUG=1;NDEBUG=1>
)
if(OS_GNU)
  CPMFindPackage(
    NAME MbedTLS
    GITHUB_REPOSITORY Mbed-TLS/mbedtls
    VERSION 3.6.3
    GIT_TAG 22098d41c6620ce07cf8a0134d37302355e1e5ef
    EXCLUDE_FROM_ALL YES
    SYSTEM YES
    OPTIONS
      "ENABLE_TESTING OFF"
      "ENABLE_PROGRAMS OFF"
      "DISABLE_PACKAGE_CONFIG_AND_INSTALL OFF"
      "USE_SHARED_MBEDTLS_LIBRARY OFF"
      "USE_STATIC_MBEDTLS_LIBRARY ON"
      "MBEDTLS_FATAL_WARNINGS OFF"
      "CMAKE_BUILD_TYPE Release"
  )
  set(MBEDTLS_LIBRARIES MbedTLS::mbedtls)
  get_target_property(MBEDTLS_INCLUDE_DIRS MbedTLS::mbedtls INTERFACE_INCLUDE_DIRECTORIES)

  CPMFindPackage(
    NAME curl
    GITHUB_REPOSITORY curl/curl
    VERSION 8.13.0
    GIT_TAG 1c3149881769e7bd79b072e48374e4c2b3678b2f
    EXCLUDE_FROM_ALL YES
    SYSTEM YES
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

  find_package(Freetype REQUIRED)
  find_package(Fontconfig REQUIRED)
  add_library(_juce_dependencies INTERFACE)
  target_link_libraries(_juce_dependencies INTERFACE
   CURL::libcurl Freetype::Freetype Fontconfig::Fontconfig)
  target_link_libraries(juce_lib PRIVATE _juce_dependencies)
  target_link_libraries(juce_clap_hosting_lib PRIVATE _juce_dependencies)
endif()
# sord/serd don't play well with UBSan
if(ZRYTHM_ENABLE_SANITIZER_UNDEFINED_BEHAVIOR)
  foreach(cur_lib juce_lib juce_clap_hosting_lib)
    if(TARGET ${cur_lib})
      target_compile_options(${cur_lib} PRIVATE -fno-sanitize=undefined)
      target_link_options(${cur_lib} PRIVATE -fno-sanitize=undefined)
    endif()
  endforeach()
endif()

add_library(_juce_libs INTERFACE)
target_link_libraries(_juce_libs INTERFACE juce_lib)
if(ZRYTHM_WITH_JUCE_CLAP_HOSTING)
  target_link_libraries(_juce_libs INTERFACE juce_clap_hosting_lib)
endif()

add_library(zrythm::juce_libs ALIAS _juce_libs)

add_subdirectory(kissfft)
add_subdirectory(midilib)
add_subdirectory(qm-dsp)
add_subdirectory(rubberband)

set(WITH_OPENMP OFF)
set(BUILD_TESTS OFF)
set(BUILD_LSR_TESTS OFF)
add_subdirectory(soxr EXCLUDE_FROM_ALL)
add_library(zrythm::soxr ALIAS soxr)

add_subdirectory(weakjack)
add_subdirectory(zita-resampler)
