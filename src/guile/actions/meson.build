# SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

_guile_snarfable_srcs = [
  'channel_send_action.c',
  'port_connection_action.c',
  'tracklist_selections_action.c',
  'undo_manager.c',
  ]

foreach snarfable_src : _guile_snarfable_srcs
  guile_snarfable_srcs += [
    join_paths (
      meson.current_source_dir(), snarfable_src),
    ]
endforeach
