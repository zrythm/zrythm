# SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_utils_lib STATIC)

target_sources(zrythm_utils_lib
  PRIVATE
    algorithms.h
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
    initializable_object.h
    io.h
    io.cpp
    icloneable.h
    iserializable.h
    jack.h
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
    monotonic_time_provider.h
    mpmc_queue.h
    networking.h
    networking.cpp
    note_type.h
    note_type.cpp
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
    symap.h
    symap.cpp
    traits.h
    types.h
    utils.h
    uuid_identifiable_object.h
    uuid_identifiable_object.cpp
    # vamp.h
    # vamp.cpp
    variant_helpers.h
    views.h
    yaml.h
    yaml.cpp)

set_target_properties(zrythm_utils_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_precompile_headers(zrythm_utils_lib PUBLIC
  $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/utils.h>)

target_link_libraries(zrythm_utils_lib
  PUBLIC
  zrythm::include_dirs
  ${zrythm_link_libs}
  zrythm::all_compile_options
)

add_library(zrythm::utils_lib ALIAS zrythm_utils_lib)