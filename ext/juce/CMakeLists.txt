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
  #modules/juce_dsp/juce_dsp.cpp
)

set(juce_public_defs
  JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
  JUCE_USE_MP3AUDIOFORMAT=1
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

set(juce_dependencies)

if(APPLE)
  find_library(FOUNDATION_LIBRARY Foundation)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(APPKIT_LIBRARY AppKit)
  find_library(IOKIT_LIBRARY IOKit)
  find_library(SECURITY_LIBRARY Security)
  find_library(AUDIO_TOOLBOX_LIBRARY AudioToolbox)
  list(APPEND juce_dependencies
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
  list(APPEND juce_dependencies
    comdlg32 gdi32 imm32 ole32 oleaut32 shlwapi uuid version wininet winmm 
    ws2_32)
elseif(${OS_GNU})
  find_package(CURL REQUIRED)
  list(APPEND juce_dependencies CURL::libcurl)
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
target_link_libraries(juce INTERFACE ${juce_dependencies})
target_compile_features(juce PRIVATE cxx_std_17)

add_library(juce::juce ALIAS juce)
