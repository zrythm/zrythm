# SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_utils_lib STATIC)

target_sources(zrythm_utils_lib
  PRIVATE
    algorithms.h
    algorithms.cpp
    audio.h
    audio.cpp
    audio_file.h
    audio_file.cpp
    backtrace.h
    backtrace.cpp
    base64.h
    chromaprint.h
    chromaprint.cpp
    color.h
    color.cpp
    compression.h
    compression.cpp
    concurrency.h
    cpu_windows.h
    cpu_windows.cpp
    datetime.h
    datetime.cpp
    directory_manager.h
    directory_manager.cpp
    dsp.h
    dsp.cpp
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
    icloneable.h
    iserializable.h
    json.h
    json.cpp
    logger.h
    logger.cpp
    math.h
    math.cpp
    mem.h
    mem.cpp
    midi.h
    midi.cpp
    mpmc_queue.h
    networking.h
    networking.cpp
    note_type.h
    note_type.cpp
    object_factory.h
    object_pool.h
    objects.h
    objects.cpp
    pcg_rand.h
    pcg_rand.cpp
    progress_info.h
    progress_info.cpp
    qt.h
    resampler.h
    resampler.cpp
    ring_buffer.h
    rt_thread_id.h
    rt_thread_id.cpp
    string.h
    string.cpp
    string_array.h
    string_array.cpp
    symap.h
    symap.cpp
    # vamp.h
    # vamp.cpp
    windows.h
    windows.cpp
    windows_errors.h
    windows_errors.cpp
    yaml.h
    yaml.cpp)

target_include_directories(zrythm_utils_lib PUBLIC ${zrythm_include_dirs})
target_compile_options(zrythm_utils_lib PUBLIC ${zrythm_compile_args})
target_compile_definitions(zrythm_utils_lib PUBLIC ${zrythm_definitions})
target_link_libraries(zrythm_utils_lib PUBLIC ${zrythm_link_libs} Qt6::Core)
