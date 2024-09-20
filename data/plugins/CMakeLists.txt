# SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if(NOT ZRYTHM_BUNDLED_PLUGINS)
  return()
endif()

set(plugins
  "Compressor|Basic compressor|CompressorPlugin"
  "Cubic Distortion|Cubic distortion|DistortionPlugin"
  "Flanger|Flanger effect|FlangerPlugin"
  "Gate Stereo|Stereo gate|GatePlugin"
  "Highpass Filter|2nd-order Butterworth highpass filter|HighpassPlugin"
  "Lowpass Filter|2nd-order Butterworth lowpass filter|LowpassPlugin"
  "White Noise|White noise generator|GeneratorPlugin"
  "Parametric EQ|Parametric equalizer with high/low shelves|ParaEQPlugin"
  "Peak Limiter|1176 Peak limiter|LimiterPlugin"
  "Phaser|Phaser effect|PhaserPlugin"
  "Smooth Delay|Delay plugin|DelayPlugin"
  "Triple Synth|Synth with 3 detuned oscillators|InstrumentPlugin"
  "Wah4|Wah pedal|FilterPlugin"
  "Zita Rev1|Zita reverb algorithm|ReverbPlugin")

set(uri_prefix "https://lv2.zrythm.org/faust-builtin")

set(pl_generate_targets)
set(bundled_plugins_for_tests)
set(bundled_plugins_for_tests_tgts)

foreach(plugin_info ${plugins})
  string(REPLACE "|" ";" plugin_info_list ${plugin_info})
  list(GET plugin_info_list 0 pl_name)
  list(GET plugin_info_list 1 pl_descr)
  list(GET plugin_info_list 2 pl_type)
  
  string(REPLACE " " "_" pl_underscored_name ${pl_name})
  string(TOLOWER ${pl_underscored_name} pl_underscored_name)

  if(FAUST2LV2_EXECUTABLE)
    set(pl_author "Zrythm DAW")
    configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/${pl_underscored_name}.dsp.in"
      "${CMAKE_CURRENT_BINARY_DIR}/${pl_underscored_name}.dsp"
      @ONLY
    )

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${pl_underscored_name}.lv2.gen"
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen-lv2.py
        ${FAUST2LV2_EXECUTABLE}
        "${CMAKE_CURRENT_BINARY_DIR}/${pl_underscored_name}.dsp"
        "${uri_prefix}"
        "${CMAKE_CURRENT_BINARY_DIR}/${pl_underscored_name}.lv2.gen.private"
        "${CMAKE_CURRENT_BINARY_DIR}/${pl_underscored_name}.lv2.gen"
        ${pl_type}
        "${CMAKE_CURRENT_SOURCE_DIR}/zrythm-utils.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/generated"
      DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${pl_underscored_name}.dsp"
              "${CMAKE_CURRENT_SOURCE_DIR}/zrythm-utils.lib"
              "${CMAKE_CURRENT_SOURCE_DIR}/gen-lv2.py"
      COMMENT "Generating LV2 plugin ${pl_name}"
    )
    list(APPEND pl_generate_targets "${CMAKE_CURRENT_BINARY_DIR}/${pl_underscored_name}.lv2.gen")
  endif()

  set(pl_generated_dir "${CMAKE_CURRENT_SOURCE_DIR}/generated/${pl_underscored_name}.lv2")
  set(pl_install_dir "${zrythm_libdir}/lv2/${pl_underscored_name}.lv2")

  set(pl_cpp "${pl_generated_dir}/${pl_underscored_name}.cpp")
  if(EXISTS ${pl_cpp})
    find_package(Boost REQUIRED CONFIG)

    set(pl_link_args)
    set(pl_cpp_args)
    set(pl_deps
      Boost::boost
      PkgConfig::LV2
    )
    if(compiler_is_gcc AND OS_GNU AND ZRYTHM_BUNDLED_PLUGINS_WITH_STATIC_LINKING)
      list(APPEND pl_link_args -static-libgcc -static-libstdc++)
    endif()
    if(MSVC)
      list(APPEND pl_deps dirent::dirent)
    endif()

    add_library(${pl_underscored_name} MODULE ${pl_cpp})
    set_target_properties(${pl_underscored_name} PROPERTIES
      PREFIX ""
      CXX_VISIBILITY_PRESET hidden
    )
    target_link_libraries(${pl_underscored_name} PRIVATE ${pl_deps})
    target_link_options(${pl_underscored_name} PRIVATE ${pl_link_args})
    target_compile_options(${pl_underscored_name} PRIVATE
      ${full_optimization_args}
    )
    set_property(TARGET ${pl_underscored_name} PROPERTY CXX_STANDARD 11)
    install(TARGETS ${pl_underscored_name} DESTINATION ${pl_install_dir})

    configure_file(
      "${pl_generated_dir}/manifest.ttl.in"
      "${CMAKE_CURRENT_BINARY_DIR}/${pl_underscored_name}-manifest.ttl"
      @ONLY
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${pl_underscored_name}-manifest.ttl"
      DESTINATION ${pl_install_dir}
      RENAME "manifest.ttl"
    )

    configure_file(
      "${pl_generated_dir}/${pl_underscored_name}.ttl.in"
      "${CMAKE_CURRENT_BINARY_DIR}/${pl_underscored_name}.ttl"
      @ONLY
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${pl_underscored_name}.ttl"
      DESTINATION ${pl_install_dir}
    )

    set(pl_generated_dir_for_tests "${CMAKE_CURRENT_BINARY_DIR}/${pl_underscored_name}.lv2")
    add_custom_command(
      OUTPUT "${pl_generated_dir_for_tests}"
      COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/gen-lv2-bundle-for-test.py"
        "${pl_generated_dir_for_tests}"
        "${CMAKE_CURRENT_BINARY_DIR}/${pl_underscored_name}.ttl"
        "${CMAKE_CURRENT_BINARY_DIR}/${pl_underscored_name}-manifest.ttl"
        "$<TARGET_FILE:${pl_underscored_name}>"
      DEPENDS ${pl_underscored_name}
      COMMENT "Generating LV2 bundle for ${pl_name} for tests"
    )
    add_custom_target(${pl_underscored_name}-tgt-for-tests
      DEPENDS "${pl_generated_dir_for_tests}")
    set(${pl_underscored_name}-tgt-for-tests ${pl_generated_dir_for_tests})

    list(APPEND bundled_plugins_for_tests
      "${pl_underscored_name}|${uri_prefix}/${pl_underscored_name}|${pl_generated_dir_for_tests}"
    )
    list(APPEND bundled_plugins_for_tests_tgts ${pl_underscored_name}-tgt-for-tests)
  else()
    message(WARNING "${pl_cpp} not found. Run `cmake --build . --target gen-faust-plugins` to generate it")
  endif()
endforeach()

if(FAUST2LV2_EXECUTABLE)
  add_custom_target(gen-faust-plugins
    COMMENT "Generating faust plugins"
    DEPENDS ${pl_generate_targets}
  )
endif()

set(BUNDLED_PLUGINS_FOR_TESTS "${bundled_plugins_for_tests}" PARENT_SCOPE)
set(BUNDLED_PLUGINS_FOR_TESTS_TGTS ${bundled_plugins_for_tests_tgts} PARENT_SCOPE)
