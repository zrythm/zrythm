# SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
#
# This file is part of Zrythm
#
# Zrythm is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Zrythm is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.

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
