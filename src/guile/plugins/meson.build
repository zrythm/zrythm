# SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

_guile_snarfable_srcs = [
  'plugin.c',
  'plugin_manager.c',
  ]

foreach snarfable_src : _guile_snarfable_srcs
  guile_snarfable_srcs += [
    join_paths (
      meson.current_source_dir(), snarfable_src),
    ]
endforeach
