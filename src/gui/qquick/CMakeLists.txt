# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_gui_lib
  PUBLIC
    FILE_SET HEADERS
    FILES
      fade_overlay_canvas_item.h
      fade_overlay_canvas_renderer.h
      audio_region_waveform_canvas_item.h
      meter_processor.h
      qfuture_qml_wrapper.h
      qml_utils.h
      spectrum_analyzer_processor.h
      waveform_canvas_item.h
      waveform_canvas_renderer.h
      waveform_viewer_processor.h
  PRIVATE
    fade_overlay_canvas_item.cpp
    fade_overlay_canvas_renderer.cpp
    audio_region_waveform_canvas_item.cpp
    meter_processor.cpp
    qml_utils.cpp
    spectrum_analyzer_processor.cpp
    waveform_canvas_item.cpp
    waveform_canvas_renderer.cpp
    waveform_viewer_processor.cpp
)
