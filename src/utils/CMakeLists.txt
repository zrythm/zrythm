# SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_lib
  PRIVATE
    algorithms.cpp
    audio.cpp
    backtrace.cpp
    cairo.cpp
    chromaprint.cpp
    color.cpp
    compression.cpp
    cpu_windows.cpp
    datetime.cpp
    dialogs.cpp
    env.cpp
    error.cpp
    exceptions.cpp
    file.cpp
    gtest_wrapper.cpp
    gtk.cpp
    hash.cpp
    io.cpp
    json.cpp
    localization.cpp
    logger.cpp
    lsp_dsp_context.cpp
    math.cpp
    mem.cpp
    networking.cpp
    objects.cpp
    pango.cpp
    progress_info.cpp
    resampler.cpp
    resources.cpp
    rt_thread_id.cpp
    string.cpp
    string_array.cpp
    symap.cpp
    ui.cpp
    vamp.cpp
    windows.cpp
    windows_errors.cpp
    yaml.cpp
)

# optimized utils
add_library(optimized_utils_lib STATIC
  dsp.cpp
  midi.cpp
  pcg_rand.cpp
)
target_include_directories(optimized_utils_lib PRIVATE ${zrythm_include_dirs})
target_compile_options(optimized_utils_lib PRIVATE 
  ${zrythm_compile_args}
  # ${full_optimization_args}
)
target_compile_definitions(optimized_utils_lib PRIVATE ${zrythm_definitions})
target_link_libraries(optimized_utils_lib PRIVATE ${zrythm_link_libs})

if(ZRYTHM_TESTS)
  # use custom lv2apply because upstream doesn't have URID support
  add_executable(zrythm_lv2apply
    lv2apply.cpp
    objects.cpp
    symap.cpp
  )
  target_include_directories(zrythm_lv2apply PRIVATE ${zrythm_include_dirs})
  target_compile_options(zrythm_lv2apply PRIVATE ${zrythm_compile_args})
  target_compile_definitions(zrythm_lv2apply PRIVATE ${zrythm_definitions})
  target_link_libraries(zrythm_lv2apply PRIVATE 
    SndFile::sndfile
    PkgConfig::LILV
    PkgConfig::GTK4
    PkgConfig::GLIBMM
    PkgConfig::GIOMM
  )
  target_link_options(zrythm_lv2apply PRIVATE ${zrythm_link_args})
endif()