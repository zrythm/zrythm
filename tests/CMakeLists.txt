# SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if(NOT ZRYTHM_TESTS)
  return()
endif()

if("${BUNDLED_PLUGINS_FOR_TESTS_TGTS}" STREQUAL "")
  message(WARNING "Built-in plugins are not generated - generate them first")  
  return()
endif()

set(test_lv2_plugins)
set(test_lv2_plugin_libs)
add_subdirectory(lv2plugins)

find_package(GTest REQUIRED)

find_program(PIPEWIRE_EXECUTABLE pipewire)

set(TEST_ENV
    "G_TEST_SRC_ROOT_DIR=${CMAKE_SOURCE_DIR}"
    "G_TEST_SRCDIR=${CMAKE_CURRENT_SOURCE_DIR}"
    "G_TEST_BUILD_ROOT_DIR=${CMAKE_BINARY_DIR}"
    "G_TEST_BUILDDIR=${CMAKE_CURRENT_BINARY_DIR}"
    "G_SLICE=debug-blocks"
    "G_DEBUG=gc-friendly"
    "MALLOC_CHECK_=3"
    "VST_PATH=/tmp/zrythm_vst"
    "VST3_PATH=/tmp/zrythm_vst3"
    "LADSPA_PATH=/tmp/zrythm_ladspa"
    "DSSI_PATH=/tmp/zrythm_dssi"
    "CLAP_PATH=/tmp/zrythm_clap"
    "ZRYTHM_DEBUG=1"
    "Z_CURL_TIMEOUT=10"
    "G_MESSAGES_DEBUG=zrythm"
    "ZRYTHM_DSP_THREADS=4"
    "PIPEWIRE_RUNTIME_DIR=/tmp/zrythm-pipewire"
    "PIPEWIRE_DEBUG=3"
    "ASAN_OPTIONS=detect_leaks=0:verify_asan_link_order=0"
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/zrythm-test-config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/zrythm-test-config.h
    @ONLY
)

set(test_defs_list
  "DEBUG_THREADS=1"
  "TESTS_SRC_ROOT_DIR=\"${CMAKE_SOURCE_DIR}\""
  "TESTS_SRCDIR=\"${CMAKE_CURRENT_SOURCE_DIR}\""
  "TESTS_BUILDDIR=\"${CMAKE_CURRENT_BINARY_DIR}\""
  "MIDILIB_TEST_MIDI_FILES_PATH=\"${CMAKE_SOURCE_DIR}/ext/midilib/MIDIFiles\""
  )

set(HAVE_PIPEWIRE ${PIPEWIRE_EXECUTABLE})

if(PIPEWIRE_EXECUTABLE)
    list(APPEND test_defs_list
      "PIPEWIRE_SERVER_NAME=\"zrythm-pipewire-0\""
      "PIPEWIRE_BIN=\"${PIPEWIRE_EXECUTABLE}\""
      "PIPEWIRE_CONF_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/pipewire-server.conf\""
    )
endif()

find_program(RUBBERBAND_EXECUTABLE "rubberband")
if(RUBBERBAND_EXECUTABLE)
    # timestretched test file x 2
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test2.wav
        COMMAND ${RUBBERBAND_EXECUTABLE} --time 2
        ${CMAKE_CURRENT_SOURCE_DIR}/test.wav
        ${CMAKE_CURRENT_BINARY_DIR}/test2.wav
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test.wav
    )
    set(TEST_WAV2 ${CMAKE_CURRENT_BINARY_DIR}/test2.wav)
endif()

# generate 30-min audio file with sine wave
find_program(SOX_EXECUTABLE "sox")
if(SOX_EXECUTABLE)
    set(filename test_sine_ogg_30min.ogg)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${filename}
        COMMAND ${SOX_EXECUTABLE} -n -r 44100 ${CMAKE_CURRENT_BINARY_DIR}/${filename}
        synth 1800 sine 500
    )
    set(TEST_SINE_OGG_30MIN ${CMAKE_CURRENT_BINARY_DIR}/${filename})
endif()

# Internal (bundled) LV2
if(NOT LV2INFO_EXECUTABLE)
    message(FATAL_ERROR "Missing lv2info required for tests")
endif()
foreach(pl ${BUNDLED_PLUGINS_FOR_TESTS})
  string(REPLACE "|" ";" pl "${pl}")
  list(GET pl 0 name)
  list(GET pl 1 uri)
  list(GET pl 2 pl_bundle_path)
#   get_target_property(pl_bundle_path ${target} PLUGIN_BUNDLE_DIR_PATH)
  set(plugin_bundle_uri "file://${pl_bundle_path}/" )
  string(TOUPPER ${name} name_upper)
  list(APPEND test_defs_list 
    "HAVE_${name_upper}=1"
    "${name_upper}_URI=\"${uri}\""
    "${name_upper}_BUNDLE=\"${plugin_bundle_uri}\"")
endforeach()

# External LV2
foreach(pl ${ext_lv2_plugins})
  string(REPLACE "|" ";" pl "${pl}")
  list(GET pl 0 name_underscored)
  list(GET pl 1 name)
  list(GET pl 2 uri)
  if(have_ext_lv2_plugins_${name_underscored})
    string(TOUPPER ${name_underscored} name_upper)
    list(APPEND test_defs_list
    "HAVE_${name_upper}=1"
    "${name_upper}_URI=\"${uri}\""
    "${name_upper}_BUNDLE=\"${ext_lv2_plugin_bundles_${name_underscored}}\"")
    if("${name_upper}" STREQUAL "MVERB")
      set(HAVE_MVERB ON)
      set(MVERB_URI "${uri}")
    endif()
  endif()
endforeach()

# Add bundle/uri macros for test plugins
foreach(pl ${test_lv2_plugins})
  string(REPLACE "|" ";" pl "${pl}")
  list(GET pl 0 name_underscored)
  list(GET pl 1 uri)
  list(GET pl 2 bundle)
  list(GET pl 3 lib)
  string(TOUPPER ${name_underscored} name_upper)
   list(APPEND test_defs_list
    "HAVE_${name_upper}=1"
    "${name_upper}_URI=\"${uri}\""
    "${name_upper}_BUNDLE_URI=\"file://${bundle}/\"")
endforeach()

add_subdirectory(presets)

set(test_lv2apply_wavs)
if(HAVE_MVERB)
    set(file_prefix test_mixdown_midi_routed_to_instrument_track)

    # Add reverb to file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${file_prefix}_w_reverb.ogg
        COMMAND $<TARGET_FILE:zrythm_lv2apply> -i "${CMAKE_CURRENT_SOURCE_DIR}/${file_prefix}.ogg"
        -o "${CMAKE_CURRENT_BINARY_DIR}/${file_prefix}_w_reverb.ogg"
        "${MVERB_URI}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${file_prefix}.ogg"
    )
    list(APPEND test_lv2apply_wavs "${CMAKE_CURRENT_BINARY_DIR}/${file_prefix}_w_reverb.ogg")
    set(TEST_WAV3 ${CMAKE_CURRENT_BINARY_DIR}/${file_prefix}_w_reverb.ogg)

    # Add reverb to file + half gain
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${file_prefix}_w_reverb_half_gain.ogg
        COMMAND $<TARGET_FILE:zrythm_lv2apply> -i ${CMAKE_CURRENT_SOURCE_DIR}/${file_prefix}.ogg
        -c gain 50
        -o ${CMAKE_CURRENT_BINARY_DIR}/${file_prefix}_w_reverb_half_gain.ogg
        ${MVERB_URI}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${file_prefix}.ogg
    )
    list(APPEND test_lv2apply_wavs ${CMAKE_CURRENT_BINARY_DIR}/${file_prefix}_w_reverb_half_gain.ogg)
    set(TEST_WAV4 ${CMAKE_CURRENT_BINARY_DIR}/${file_prefix}_w_reverb_half_gain.ogg)
endif()

# VST
foreach(pl ${ext_vst_plugins})
 string(REPLACE "|" ";" pl "${pl}")
  list(GET pl 0 name_underscored)
  list(GET pl 1 path)
  if(have_ext_vst_plugins_${name_underscored})
    string(TOUPPER ${name_underscored} name_upper)
    list(APPEND test_defs_list
    "HAVE_${name_upper}=1"
    "${name_upper}_PATH=\"${path}\"")
    endif()
endforeach()

# VST3
foreach(pl ${ext_vst3_plugins})
 string(REPLACE "|" ";" pl "${pl}")
  list(GET pl 0 name_underscored)
  list(GET pl 1 path)
  if(have_ext_vst3_plugins_${name_underscored})
    string(TOUPPER ${name_underscored} name_upper)
    list(APPEND test_defs_list
    "HAVE_${name_upper}=1"
    "${name_upper}_PATH=\"${path}\"")
    endif()
endforeach()

set(TEST_DEFINITIONS)
foreach(def ${test_defs_list})
  string(REGEX REPLACE "(.*)=(.*)" "#define \\1 \\2" def_line "${def}")
  set(TEST_DEFINITIONS "${TEST_DEFINITIONS}\n${def_line}\n")
endforeach()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/zrythm-test-config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/zrythm-test-config.h
)

# set(TEST_LINK_ARGS -fPIC)

set(TESTS
actions/arranger_selections
actions/channel_send
actions/range
actions/transport_action
actions/undo_manager
dsp/audio_region
dsp/audio_track
dsp/automation_track
dsp/channel
dsp/channel_track
dsp/chord_track
dsp/curve
dsp/fader
dsp/graph_export
dsp/marker_track
dsp/metronome
dsp/midi_event
dsp/midi_function
dsp/midi_mapping
dsp/midi_note
dsp/midi_region
dsp/midi_track
dsp/pool
dsp/position
dsp/port
dsp/region
dsp/sample_processor
dsp/scale
dsp/snap_grid
dsp/tempo_track
dsp/track
dsp/track_processor
dsp/tracklist
dsp/transport
gui/backend/arranger_selections
gui/backend/timeline_selections
integration/memory_allocation
integration/recording
plugins/carla_discovery
plugins/carla_native_plugin
plugins/plugin
plugins/plugin_manager
project
settings/settings
utils/compression
utils/file
utils/hash
utils/math
utils/midi
utils/object_pool
utils/io
utils/string
utils/ui
utils/yaml
zrythm_app
zrythm
)

if(UNIX AND NOT APPLE)
list(APPEND TESTS
    actions/mixer_selections_action
    actions/port_connection
    actions/tracklist_selections
    actions/tracklist_selections_edit
    benchmarks/dsp
    integration/midi_file
    integration/run_graph_with_latencies
    integration/undo_redo_helm_track_creation
)
endif()

if(ZRYTHM_GUI_TESTS)
list(APPEND TESTS
    gui/widgets/region
    gui/widgets/track
)
endif()

if(CHROMAPRINT_FOUND)
list(APPEND TESTS
    dsp/exporter
)
endif()

add_library(test_helpers STATIC
helpers/exporter.cpp
helpers/project_helper.cpp
helpers/zrythm_helper.cpp
)
target_link_libraries(test_helpers PUBLIC zrythm_lib)
add_dependencies(test_helpers ${test_lv2_plugin_libs})
target_include_directories(test_helpers
  PUBLIC 
    ${all_inc} 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR})


find_package(benchmark REQUIRED)
foreach(test_path IN LISTS TESTS)
    get_filename_component(test_name ${test_path} NAME)
    string(REPLACE "/" "_" test_target ${test_path})
    if(${test_path} MATCHES "benchmarks/")
        if(benchmark_FOUND)
            set(benchmark_target_name "${test_target}_benchmark")
            add_executable(${benchmark_target_name} ${test_path}.cpp)
            target_link_libraries(${benchmark_target_name} PRIVATE 
                zrythm_lib 
                test_helpers
                benchmark::benchmark
                benchmark::benchmark_main
                )
            
            add_test(${benchmark_target_name} ${benchmark_target_name})
            set_tests_properties(${benchmark_target_name} PROPERTIES 
            ENVIRONMENT "${TEST_ENV};ZRYTHM_BENCHMARKING=1"
            TIMEOUT 680
            )
            add_dependencies(${benchmark_target_name} ${BUNDLED_PLUGINS_FOR_TESTS_TGTS})
        endif()
    else()
        set(test_target_name ${test_target}_test)
        add_executable(${test_target_name} ${test_path}.cpp)
        target_link_libraries(${test_target_name} PRIVATE 
            zrythm_lib 
            test_helpers
            GTest::gtest
            GTest::gtest_main
        )

        add_test(${test_target_name} ${test_target_name})
        set_tests_properties(${test_target_name} PROPERTIES 
            ENVIRONMENT "${TEST_ENV}"
            TIMEOUT 240
        )
        add_dependencies(${test_target_name} ${BUNDLED_PLUGINS_FOR_TESTS_TGTS})
    endif()
endforeach()