# SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_gui_lib
  PRIVATE
    carla_native_plugin.h
    carla_native_plugin.cpp
    plugin.h
    plugin.cpp
    plugin_factory.h
    plugin_span.h
    plugin_span.cpp
    port_span.h
    quantize_options.h
    quantize_options.cpp
    snap_grid.h
    snap_grid.cpp
)
