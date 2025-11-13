# SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_utils_lib STATIC)

target_sources(zrythm_utils_lib
  PRIVATE
    audio.cpp
    audio_file.cpp
    backtrace.cpp
    chromaprint.cpp
    color.cpp
    compression.cpp
    cpu_windows.cpp
    datetime.cpp
    directory_manager.cpp
    dsp.cpp
    dsp_context.cpp
    env.cpp
    exceptions.cpp
    file_path_list.cpp
    gtest_wrapper.cpp
    hash.cpp
    io.cpp
    logger.cpp
    math.cpp
    mem.cpp
    midi.cpp
    networking.cpp
    note_type.cpp
    pcg_rand.cpp
    progress_info.cpp
    resampler.cpp
    rt_thread_id.cpp
    utf8_string.cpp
    symap.cpp
    uuid_identifiable_object.cpp
    # vamp.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ".."
    FILES
      algorithms.h
      audio.h
      audio_file.h
      backtrace.h
      base64.h
      bidirectional_map.h
      chromaprint.h
      color.h
      compression.h
      concurrency.h
      cpu_windows.h
      datetime.h
      directory_manager.h
      dsp.h
      dsp_context.h
      env.h
      exceptions.h
      expandable_tick_range.h
      file_path_list.h
      gtest_wrapper.h
      hash.h
      io.h
      icloneable.h
      jack.h
      logger.h
      math.h
      mem.h
      midi.h
      monotonic_time_provider.h
      mpmc_queue.h
      networking.h
      note_type.h
      object_pool.h
      pcg_rand.h
      playback_cache_scheduler.h
      progress_info.h
      qt.h
      ranges.h
      resampler.h
      ring_buffer.h
      rt_thread_id.h
      serialization.h
      utf8_string.h
      symap.h
      thread_safe_fftw.h
      traits.h
      types.h
      units.h
      utils.h
      uuid_identifiable_object.h
      # vamp.h
      variant_helpers.h
      views.h
)

set_target_properties(zrythm_utils_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
  VERIFY_INTERFACE_HEADER_SETS ${ZRYTHM_VERIFY_INTERFACE_HEADER_SETS}
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
