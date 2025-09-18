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

# some versions of cmake require at least 1 source file for static libs...
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/empty.cpp" "// Empty file\n")
target_sources(juce_lib PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/empty.cpp")

CPMGetPackage(juce)

target_link_libraries(juce_lib
  PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
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
  CPMGetPackage(MbedTLS)
  set(MBEDTLS_LIBRARIES MbedTLS::mbedtls)
  get_target_property(MBEDTLS_INCLUDE_DIRS MbedTLS::mbedtls INTERFACE_INCLUDE_DIRECTORIES)

  CPMGetPackage(curl)

  find_package(Freetype REQUIRED)
  find_package(Fontconfig REQUIRED)
  add_library(_juce_dependencies INTERFACE)
  target_link_libraries(_juce_dependencies INTERFACE
   CURL::libcurl Freetype::Freetype Fontconfig::Fontconfig)
  target_link_libraries(juce_lib PRIVATE _juce_dependencies)
endif()
# sord/serd don't play well with UBSan
if(ZRYTHM_ENABLE_SANITIZER_UNDEFINED_BEHAVIOR)
  if(TARGET juce_lib)
    target_compile_options(juce_lib PRIVATE -fno-sanitize=undefined)
    target_link_options(juce_lib PRIVATE -fno-sanitize=undefined)
  endif()
endif()

add_library(_juce_libs INTERFACE)
target_link_libraries(_juce_libs INTERFACE juce_lib)

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
