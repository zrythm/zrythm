# SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_gui_lib
  PRIVATE
    audio_port.h
    audio_port.cpp
    carla_native_plugin.h
    carla_native_plugin.cpp
    control_port.h
    control_port.cpp
    cv_port.h
    cv_port.cpp
    midi_port.h
    midi_port.cpp
    modulator_macro_processor.h
    modulator_macro_processor.cpp
    plugin.h
    plugin.cpp
    plugin_factory.h
    plugin_span.h
    plugin_span.cpp
    port.h
    port.cpp
    port_all.h
    port_backend.h
    port_span.h
    quantize_options.h
    quantize_options.cpp
    snap_grid.h
    snap_grid.cpp
)
