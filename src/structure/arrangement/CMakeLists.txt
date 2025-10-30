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
    audio_clip_editor.cpp
    audio_region.cpp
    audio_source_object.cpp
    automation_editor.cpp
    automation_point.cpp
    automation_region.cpp
    bounded_object.cpp
    chord_editor.cpp
    chord_object.cpp
    chord_region.cpp
    editor_settings.cpp
    fadeable_object.cpp
    loopable_object.cpp
    marker.cpp
    midi_note.cpp
    midi_region.cpp
    named_object.cpp
    piano_roll.cpp
    region_renderer.cpp
    scale_object.cpp
    timeline.cpp
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
      audio_clip_editor.h
      audio_region.h
      audio_source_object.h
      automation_editor.h
      automation_point.h
      automation_region.h
      bounded_object.h
      chord_editor.h
      chord_object.h
      chord_region.h
      colored_object.h
      editor_settings.h
      fadeable_object.h
      loopable_object.h
      marker.h
      midi_note.h
      midi_region.h
      muteable_object.h
      named_object.h
      piano_roll.h
      region_renderer.h
      scale_object.h
      timeline.h
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
  IMPORTS TARGET zrythm_dsp_lib
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmArrangement
)

target_sources(zrythm_gui_lib
  PRIVATE
    audio_function.h
    audio_function.cpp
    arranger_object_span.h
    arranger_object_span.cpp
    automation_function.h
    automation_function.cpp
    midi_function.h
    midi_function.cpp
    # region_link_group.h
    # region_link_group.cpp
    # region_link_group_manager.h
    # region_link_group_manager.cpp
)
