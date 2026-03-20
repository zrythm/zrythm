# SPDX-FileCopyrightText: © 2022, 2024, 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if(NOT ZRYTHM_BUNDLED_PLUGINS)
  return()
endif()

# Disable AUTOMOC for all targets in this directory
# These are pure JUCE plugins without Qt meta-objects
set(CMAKE_AUTOMOC OFF)

# Set VST3 copy directory for all plugins (used when COPY_PLUGIN_AFTER_BUILD is TRUE)
set_directory_properties(PROPERTIES
  JUCE_VST3_COPY_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/vst3"
)

# Plugin definitions: "Name|Description|Type|Code"
# See JUCE VST3_CATEGORIES property for available types
set(plugins
  "Compressor|Basic compressor|Fx Dynamics|Cmpr"
  "Cubic Distortion|Cubic distortion|Fx Distortion|CbDs"
  "Flanger|Flanger effect|Fx Modulation|Flng"
  "Gate Stereo|Stereo gate|Fx Dynamics|GtSt"
  "Highpass Filter|2nd-order Butterworth highpass filter|Fx Filter|HgPs"
  "Lowpass Filter|2nd-order Butterworth lowpass filter|Fx Filter|LwPs"
  "White Noise|White noise generator|Fx Generator|WhNs"
  "Parametric EQ|Parametric equalizer with high/low shelves|Fx EQ|PrEQ"
  "Peak Limiter|1176 Peak limiter|Fx Dynamics|PkLm"
  "Phaser|Phaser effect|Fx Modulation|Phsr"
  "Smooth Delay|Delay plugin|Fx Delay|SmDl"
  "Triple Synth|Synth with 3 detuned oscillators|Instrument Synth|TrSy"
  "Wah4|Wah pedal|Fx Filter|Wah4"
  "Zita Rev1|Zita reverb algorithm|Fx Reverb|ZtR1")

# Manufacturer code (4-char, at least one uppercase)
set(plugin_manufacturer_code "Zryt")

set(pl_generate_targets)
set(bundled_plugins_for_tests)
set(bundled_plugins_for_tests_tgts)
set(bundled_vst3_search_paths)

# Output directory for generated JUCE source
set(generated_juce_dir "${CMAKE_CURRENT_SOURCE_DIR}/generated-juce")

foreach(plugin_info ${plugins})
  string(REPLACE "|" ";" plugin_info_list ${plugin_info})
  list(GET plugin_info_list 0 pl_name)
  list(GET plugin_info_list 1 pl_descr)
  list(GET plugin_info_list 2 pl_type)
  list(GET plugin_info_list 3 pl_code)

  string(REPLACE " " "_" pl_underscored_name ${pl_name})
  string(TOLOWER ${pl_underscored_name} pl_underscored_name)

  # Determine if this is an instrument (needs MIDI and polyphony)
  if(pl_type MATCHES "Instrument")
    set(is_synth TRUE)
    set(needs_midi_input TRUE)
    set(FAUST_POLYPHONY_ARGS "--midi")
  else()
    set(is_synth FALSE)
    set(needs_midi_input FALSE)
    set(FAUST_POLYPHONY_ARGS)
  endif()

  # Configure DSP file from template
  set(pl_author "Zrythm DAW")
  set(configured_dsp "${CMAKE_CURRENT_BINARY_DIR}/${pl_underscored_name}.dsp")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${pl_underscored_name}.dsp.in"
    "${configured_dsp}"
    @ONLY
  )

  # Generate JUCE source using gen-juce.py
  set(pl_generated_cpp "${generated_juce_dir}/${pl_underscored_name}/FaustPluginProcessor.cpp")

  if(FAUST2JUCE_EXECUTABLE OR EXISTS ${pl_generated_cpp})
    if(FAUST2JUCE_EXECUTABLE)
      add_custom_command(
        OUTPUT "${pl_generated_cpp}"
        COMMAND ${Python3_EXECUTABLE}
          "${CMAKE_CURRENT_SOURCE_DIR}/gen-juce.py"
          "${configured_dsp}"
          "-o" "${generated_juce_dir}"
          "-j" "${JUCE_SOURCE_DIR}/modules"
          "-u" "${CMAKE_CURRENT_SOURCE_DIR}/zrythm-utils.lib"
          ${FAUST_POLYPHONY_ARGS}
          "--no-gui"
          "--faust2juce" "${FAUST2JUCE_EXECUTABLE}"
        DEPENDS
          "${configured_dsp}"
          "${CMAKE_CURRENT_SOURCE_DIR}/zrythm-utils.lib"
          "${CMAKE_CURRENT_SOURCE_DIR}/gen-juce.py"
        COMMENT "Generating JUCE plugin source for ${pl_name}"
      )
      list(APPEND pl_generate_targets "${pl_generated_cpp}")
    endif()

    if(EXISTS ${pl_generated_cpp})
      # Build VST3 using JUCE CMake API
      juce_add_plugin(${pl_underscored_name}_plugin
        PLUGIN_MANUFACTURER_CODE ${plugin_manufacturer_code}
        PLUGIN_CODE ${pl_code}
        FORMATS VST3
        PRODUCT_NAME "${pl_name}"
        IS_SYNTH ${is_synth}
        NEEDS_MIDI_INPUT ${needs_midi_input}
        COPY_PLUGIN_AFTER_BUILD TRUE
        DESCRIPTION "${pl_descr}"
        VST3_CATEGORIES "${pl_type}"
      )

      target_sources(${pl_underscored_name}_plugin
        PRIVATE
          "${pl_generated_cpp}"
      )

      target_link_libraries(${pl_underscored_name}_plugin
        PUBLIC
          juce::juce_recommended_config_flags
      )

      if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
        target_link_libraries(${pl_underscored_name}_plugin
          PUBLIC juce::juce_recommended_lto_flags
        )
      endif()

      target_compile_definitions(${pl_underscored_name}_plugin
        PUBLIC
          JUCE_WEB_BROWSER=0
          JUCE_USE_CURL=0
          JUCE_VST3_CAN_REPLACE_VST2=0
          # Faust-based plugins have long parameter IDs hitting AAX-related assertions
          JUCE_DISABLE_CAUTIOUS_PARAMETER_ID_CHECKING=1
      )

      if(is_synth)
        target_compile_definitions(${pl_underscored_name}_plugin
        PUBLIC
          MIDICTRL
        )
      endif()

      # Add to test dependencies (use build directory path for tests)
      list(APPEND bundled_plugins_for_tests
        "${pl_underscored_name}|vst3|${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/vst3/${pl_name}.vst3"
      )
      list(APPEND bundled_plugins_for_tests_tgts ${pl_underscored_name}_plugin_VST3)
    else()
      message(WARNING "${pl_generated_cpp} not found. Run `cmake --build . --target gen-faust-plugins` to generate it")
    endif()
  endif()
endforeach()

# Generation target
if(FAUST2JUCE_EXECUTABLE)
  add_custom_target(gen-faust-plugins
    COMMENT "Generating Faust JUCE plugins"
    DEPENDS ${pl_generate_targets}
  )
endif()

# Install all VST3 plugins from the copy directory
install(
  DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/vst3"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  COMPONENT Runtime
)

set(BUNDLED_PLUGINS_FOR_TESTS "${bundled_plugins_for_tests}" PARENT_SCOPE)
set(BUNDLED_PLUGINS_FOR_TESTS_TGTS ${bundled_plugins_for_tests_tgts} PARENT_SCOPE)
set(BUNDLED_VST3_SEARCH_PATHS "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/vst3" PARENT_SCOPE)
