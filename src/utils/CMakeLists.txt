# SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_utils_lib STATIC)

target_sources(zrythm_utils_lib
  PRIVATE
    app_settings.cpp
    audio.cpp
    audio_file.cpp
    audio_file_writer.cpp
    backtrace.cpp
    chromaprint.cpp
    color.cpp
    compression.cpp
    datetime.cpp
    debouncer.cpp
    directory_manager.cpp
    dsp.cpp
    dsp_context.cpp
    env.cpp
    exceptions.cpp
    expandable_tick_range.cpp
    file_path_list.cpp
    gtest_wrapper.cpp
    hash.cpp
    io_utils.cpp
    logger.cpp
    math_utils.cpp
    mem.cpp
    midi.cpp
    networking.cpp
    note_type.cpp
    pcg_rand.cpp
    playback_cache_scheduler.cpp
    resampler.cpp
    rt_thread_id.cpp
    utf8_string.cpp
    symap.cpp
    threads.cpp
    uuid_identifiable_object.cpp
    # vamp.cpp
    version.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ".."
    FILES
      algorithms.h
      app_settings.h
      audio.h
      audio_file.h
      audio_file_writer.h
      backtrace.h
      base64.h
      bidirectional_map.h
      chromaprint.h
      color.h
      compression.h
      concurrency.h
      datetime.h
      debouncer.h
      directory_manager.h
      dsp.h
      dsp_context.h
      enum_utils.h
      env.h
      exceptions.h
      expandable_tick_range.h
      file_path_list.h
      format.h
      format_boost.h
      format_juce.h
      format_qt.h
      gtest_wrapper.h
      hash.h
      io_utils.h
      icloneable.h
      isettings_backend.h
      jack.h
      logger.h
      math_utils.h
      mem.h
      midi.h
      monotonic_time_provider.h
      mpmc_queue.h
      networking.h
      note_type.h
      object_pool.h
      optional_ref.h
      pcg_rand.h
      playback_cache_scheduler.h
      qsettings_backend.h
      qt.h
      raii_utils.h
      ranges.h
      resampler.h
      ring_buffer.h
      rt_thread_id.h
      serialization.h
      utf8_string.h
      symap.h
      thread_safe_fftw.h
      threads.h
      traits.h
      types.h
      units.h
      utils.h
      uuid_identifiable_object.h
      # vamp.h
      variant_helpers.h
      version.h
      views.h
)

set_target_properties(zrythm_utils_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
  VERIFY_INTERFACE_HEADER_SETS ${ZRYTHM_VERIFY_INTERFACE_HEADER_SETS}
)

target_precompile_headers(zrythm_utils_lib
  PUBLIC
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/utils.h>
    # Formatter specialization for standard types breaks unless this is
    # included in PCH
    $<$<COMPILE_LANGUAGE:CXX>:fmt/std.h>
)

target_link_libraries(zrythm_utils_lib
  PUBLIC
  zrythm::include_dirs
  ${zrythm_link_libs}
  zrythm::all_compile_options
)

# create a plugin so that types get exposed to QML
qt_add_qml_module(zrythm_utils_lib
  URI ZrythmUtils
  VERSION 1.0
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmUtils
)
target_precompile_headers(zrythm_utils_libplugin REUSE_FROM zrythm_utils_lib)

add_library(zrythm::utils_lib ALIAS zrythm_utils_lib)
