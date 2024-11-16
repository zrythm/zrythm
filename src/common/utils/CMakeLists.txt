# SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_lib
  PRIVATE
    algorithms.h
    algorithms.cpp
    audio.h
    audio.cpp
    backtrace.h
    backtrace.cpp
    base64.h
    chromaprint.h
    chromaprint.cpp
    color.h
    color.cpp
    compression.h
    compression.cpp
    cpu_windows.h
    cpu_windows.cpp
    datetime.h
    datetime.cpp
    directory_manager.h
    directory_manager.cpp
    dsp_context.h
    dsp_context.cpp
    env.h
    env.cpp
    exceptions.h
    exceptions.cpp
    file_path_list.h
    file_path_list.cpp
    gtest_wrapper.h
    gtest_wrapper.cpp
    hash.h
    hash.cpp
    io.h
    io.cpp
    json.h
    json.cpp
    localization.cpp
    logger.h
    logger.cpp
    math.h
    math.cpp
    mem.h
    mem.cpp
    networking.h
    networking.cpp
    objects.h
    objects.cpp
    progress_info.h
    progress_info.cpp
    qt.h
    resampler.h
    resampler.cpp
    rt_thread_id.h
    rt_thread_id.cpp
    string.h
    string.cpp
    string_array.h
    string_array.cpp
    symap.h
    symap.cpp
    ui.h
    ui.cpp
    # vamp.h
    # vamp.cpp
    windows.h
    windows.cpp
    windows_errors.h
    windows_errors.cpp
    yaml.h
    yaml.cpp)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/localization.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/localization.h"
  @ONLY)
target_sources(zrythm_lib
  PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/localization.h")

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
target_link_libraries(optimized_utils_lib PRIVATE ${zrythm_link_libs} Qt6::Core)

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
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
  )
  target_link_options(zrythm_lv2apply PRIVATE ${zrythm_link_args})
endif()