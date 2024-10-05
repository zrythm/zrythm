# SPDX-FileCopyrightText: © 2019-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_lib
  PRIVATE
    cached_plugin_descriptors.cpp
    carla_discovery.cpp
    carla_native_plugin.cpp
    collections.cpp
    plugin.cpp
    plugin_descriptor.cpp
    plugin_gtk.cpp
    plugin_identifier.cpp
    plugin_manager.cpp
    )
