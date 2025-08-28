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
    arranger_object.h
    arranger_object.cpp
    arranger_object_all.h
    arranger_object_factory.h
    arranger_object_fwd.h
    arranger_object_list_model.h
    arranger_object_owner.h
    audio_region.h
    audio_region.cpp
    audio_source_object.h
    audio_source_object.cpp
    automation_point.h
    automation_point.cpp
    automation_region.h
    automation_region.cpp
    bounded_object.h
    bounded_object.cpp
    chord_object.h
    chord_object.cpp
    chord_region.h
    chord_region.cpp
    colored_object.h
    fadeable_object.h
    fadeable_object.cpp
    loopable_object.h
    loopable_object.cpp
    marker.h
    marker.cpp
    midi_note.h
    midi_note.cpp
    midi_region.h
    midi_region.cpp
    muteable_object.h
    named_object.h
    named_object.cpp
    region.h
    region.cpp
    scale_object.h
    scale_object.cpp
)
target_precompile_headers(zrythm_arrangement_lib PUBLIC
  $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/arranger_object_all.h>
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
