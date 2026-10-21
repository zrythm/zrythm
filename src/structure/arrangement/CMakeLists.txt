# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_arrangement_lib STATIC)

target_link_libraries(zrythm_arrangement_lib
  PUBLIC
    ${zrythm_link_libs}
    zrythm_dsp_lib
    zrythm::include_dirs
    zrythm::all_compile_options
)
target_sources(zrythm_arrangement_lib
  PRIVATE
    arranger_object.cpp
    arranger_object_list_model.cpp
    audio_clip.cpp
    audio_function.cpp
    audio_source_object.cpp
    audio_transform_entry_list_model.cpp
    automation_point.cpp
    automation_clip.cpp
    automation_function.cpp
    chord_object.cpp
    chord_clip.cpp
    clip.cpp
    colored_object.cpp
    fadeable_object.cpp
    marker.cpp
    midi_control_event.cpp
    midi_note.cpp
    midi_clip.cpp
    midi_function.cpp
    muteable_object.cpp
    named_object.cpp
    clip_renderer.cpp
    scale_object.cpp
    tempo_object.cpp
    tempo_object_manager.cpp
    time_signature_object.cpp
    timeline_data_provider.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "../.."
    FILES
      arranger_object.h
      arranger_object_all.h
      arranger_object_factory.h
      arranger_object_fwd.h
      arranger_object_list_model.h
      arranger_object_owner.h
      audio_clip.h
      audio_function.h
      audio_source_object.h
      audio_transform_entry_list_model.h
      automation_point.h
      automation_clip.h
      automation_function.h
      chord_object.h
      chord_clip.h
      clip.h
      colored_object.h
      editor_function_kind.h
      editor_function_param.h
      fadeable_object.h
      loop_segment_iterator.h
      marker.h
      midi_control_event.h
      midi_note.h
      midi_clip.h
      midi_function.h
      muteable_object.h
      named_object.h
      clip_renderer.h
      scale_object.h
      tempo_object.h
      tempo_object_manager.h
      time_signature_object.h
      timeline_data_provider.h
)

# TODO: some issues with UuidIdentifiableObject-related usage
# set_target_properties(zrythm_arrangement_lib PROPERTIES
  # UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
  # VERIFY_INTERFACE_HEADER_SETS ${ZRYTHM_VERIFY_INTERFACE_HEADER_SETS}
# )

target_precompile_headers(zrythm_arrangement_lib PUBLIC
  $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/arranger_object_factory.h>
)

# create a plugin so that types get exposed to QML
qt_add_qml_module(zrythm_arrangement_lib
  URI ZrythmArrangement
  VERSION 1.0
  DEPENDENCIES
    QtCore # Needed for QAbstractItemModel-derived types
  IMPORTS
    TARGET zrythm_utils_lib
    TARGET zrythm_dsp_lib
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmArrangement
)
set_target_properties(zrythm_arrangement_libplugin PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)
