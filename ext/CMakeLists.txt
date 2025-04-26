# SPDX-FileCopyrightText: © 2019-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_subdirectory(backward-cpp)
add_subdirectory(crill)
add_subdirectory(dirent)

set(JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED ON)
set(JUCE_MODULES_ONLY ON)
if(OS_GNU)
  set(JUCE_NEEDS_CURL ON)
endif()
add_library(juce_lib STATIC)

# TODO try find_package() first (if not found, download)
FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 10a589619b452c261b2940767eb253171eb5a823  # 8.0.7
)
FetchContent_MakeAvailable(juce)

if(ZRYTHM_WITH_JUCE_CLAP_HOSTING)
  FetchContent_Declare(
    clap
    GIT_REPOSITORY https://github.com/free-audio/clap.git
    GIT_TAG 27f20f81dec40b930d79ef429fd35dcc2d45db5b # 1.2.2
  )
  FetchContent_MakeAvailable(clap)
  FetchContent_Declare(
    clap-helpers
    GIT_REPOSITORY https://github.com/free-audio/clap-helpers.git
    GIT_TAG 59791394dc26637d9425c2745233a979602be2a7 # main branch
  )
  FetchContent_MakeAvailable(clap-helpers)

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
      juce::juce_recommended_lto_flags
  )
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
    juce::juce_core
    juce::juce_data_structures
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    # juce::juce_recommended_warning_flags
)
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
  find_package(CURL REQUIRED)
  target_link_libraries(juce_lib PRIVATE CURL::libcurl)
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
add_subdirectory(soxr)
add_library(zrythm::soxr ALIAS soxr)

add_subdirectory(weakjack)
add_subdirectory(whereami)
add_subdirectory(zita-resampler)
add_subdirectory(typesafe)
