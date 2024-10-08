# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

set(juce_include_dirs
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/modules
)
set(juce_public_include_dirs
  ${CMAKE_CURRENT_SOURCE_DIR}
)

set(juce_sources
  modules/juce_core/juce_core.cpp
  modules/juce_audio_basics/juce_audio_basics.cpp
  modules/juce_audio_formats/juce_audio_formats.cpp
  modules/juce_audio_processors/juce_audio_processors.cpp
  modules/juce_gui_basics/juce_gui_basics.cpp
  modules/juce_graphics/juce_graphics.cpp
  modules/juce_events/juce_events.cpp
  modules/juce_data_structures/juce_data_structures.cpp
  modules/juce_gui_extra/juce_gui_extra.cpp
)

set(juce_public_defs
  JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
  JUCE_USE_MP3AUDIOFORMAT=1
  JUCE_WEB_BROWSER=0
  JUCE_PLUGINHOST_VST3=1
  JUCE_PLUGINHOST_AU=1
  JUCE_PLUGINHOST_LADSPA=1
  JUCE_PLUGINHOST_LV2=1
)

if(IS_DEBUG_BUILD)
  list(APPEND juce_public_defs
    JUCE_DEBUG=1
    DEBUG=1
  )
else()
  list(APPEND juce_public_defs
    JUCE_DEBUG=0
    DEBUG=0
  )
endif()

set(juce_private_compile_opts)
if(${compiler_is_gcc})
  list(APPEND juce_private_compile_opts
      -fno-single-precision-constant
  )
endif()

set(juce_interface_deps)
set(juce_private_deps)

if(APPLE)
  find_library(FOUNDATION_LIBRARY Foundation)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(APPKIT_LIBRARY AppKit)
  find_library(IOKIT_LIBRARY IOKit)
  find_library(SECURITY_LIBRARY Security)
  find_library(AUDIO_TOOLBOX_LIBRARY AudioToolbox)
  list(APPEND juce_interface_deps
    ${FOUNDATION_LIBRARY}
    ${COCOA_LIBRARY}
    ${APPKIT_LIBRARY}
    ${IOKIT_LIBRARY}
    ${SECURITY_LIBRARY}
    ${AUDIO_TOOLBOX_LIBRARY}
  )

  set_source_files_properties(
    ${juce_sources} PROPERTIES LANGUAGE OBJCXX
  )
elseif(WIN32)
  if(MSVC)
    # Uncomment if needed
    # list(APPEND juce_private_compile_opts /permissive)
  else()
    list(APPEND juce_private_compile_opts -fpermissive)
  endif()
  list(APPEND juce_interface_deps
    comdlg32 gdi32 imm32 ole32 oleaut32 shlwapi uuid version wininet winmm 
    ws2_32)
elseif(${OS_GNU})
  find_package(CURL REQUIRED)
  find_package(Freetype REQUIRED)
  list(APPEND juce_private_deps CURL::libcurl Freetype::Freetype)
endif()


add_library(juce_lv2_libs STATIC
  modules/juce_audio_processors/juce_audio_processors_lv2_libs.cpp
)
target_include_directories(juce_lv2_libs 
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/juce_audio_processors/format_types/LV2_SDK/lv2"
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/juce_audio_processors/format_types/LV2_SDK/serd"
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/juce_audio_processors/format_types/LV2_SDK/sord"
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/juce_audio_processors/format_types/LV2_SDK/sord/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/juce_audio_processors/format_types/LV2_SDK/sratom"
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/juce_audio_processors/format_types/LV2_SDK/lilv"
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/juce_audio_processors/format_types/LV2_SDK/lilv/src"
    ${juce_include_dirs}
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/juce_audio_processors/format_types/LV2_SDK"
)
target_compile_definitions(juce_lv2_libs PRIVATE ${juce_public_defs})
target_compile_options(juce_lv2_libs PRIVATE ${juce_private_compile_opts})
# sord/serd don't play well with UBSan
if(ZRYTHM_ENABLE_SANITIZER_UNDEFINED_BEHAVIOR)
  target_compile_options(juce_lv2_libs PRIVATE
    -fno-sanitize=undefined
  )
  target_link_options(juce_lv2_libs PRIVATE
    -fno-sanitize=undefined
  )
endif()

add_library(juce STATIC)
target_sources(juce PRIVATE ${juce_sources})
target_include_directories(juce 
  SYSTEM PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/modules
)
target_compile_definitions(juce PUBLIC ${juce_public_defs})
target_compile_options(juce PRIVATE ${juce_private_compile_opts})
target_link_libraries(juce INTERFACE ${juce_interface_deps})
target_link_libraries(juce PRIVATE ${juce_private_deps} juce_lv2_libs)
target_include_directories(juce 
  PRIVATE
    # LADSPA
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/juce_audio_processors/format_types/LADSPA_SDK"
    # VST3
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/juce_audio_processors/format_types/VST3_SDK"
  )
target_compile_features(juce PRIVATE cxx_std_17)

add_library(juce::juce ALIAS juce)
