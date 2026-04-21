# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_gui_lib
  PUBLIC
    FILE_SET HEADERS
    FILES
      arranger_grid_canvas_item.h
      arranger_grid_canvas_renderer.h
      audio_region_waveform_canvas_item.h
      fade_overlay_canvas_item.h
      fade_overlay_canvas_renderer.h
      grid_line_computer.h
      meter_processor.h
      qfuture_qml_wrapper.h
      plugin_inspector_controller.h
      qml_utils.h
      ruler_grid_canvas_item.h
      ruler_grid_canvas_renderer.h
      spectrum_analyzer_processor.h
      waveform_canvas_item.h
      waveform_canvas_renderer.h
      waveform_viewer_processor.h
  PRIVATE
    arranger_grid_canvas_item.cpp
    arranger_grid_canvas_renderer.cpp
    audio_region_waveform_canvas_item.cpp
    fade_overlay_canvas_item.cpp
    fade_overlay_canvas_renderer.cpp
    grid_line_computer.cpp
    meter_processor.cpp
    plugin_inspector_controller.cpp
    qml_utils.cpp
    ruler_grid_canvas_item.cpp
    ruler_grid_canvas_renderer.cpp
    spectrum_analyzer_processor.cpp
    waveform_canvas_item.cpp
    waveform_canvas_renderer.cpp
    waveform_viewer_processor.cpp
)
