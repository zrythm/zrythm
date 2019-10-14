# Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

dzl_srcs = [
  'dzl-animation.c',
  'dzl-bin.c',
  'dzl-child-property-action.c',
  'dzl-dock-bin.c',
  'dzl-dock-bin-edge.c',
  'dzl-dock.c',
  'dzl-dock-item.c',
  'dzl-dock-manager.c',
  'dzl-dock-paned.c',
  'dzl-dock-revealer.c',
  'dzl-dock-stack.c',
  'dzl-dock-transient-grab.c',
  'dzl-dock-widget.c',
  'dzl-frame-source.c',
  'dzl-gtk.c',
  'dzl-multi-paned.c',
  'dzl-tab.c',
  'dzl-tab-strip.c',
  'dzl-util.c',
  ]

dzl_lib = static_library (
  'dzl-lib',
  sources: dzl_srcs,
  dependencies: [
    gtk_dep,
    config_h_dep,
    ],
  include_directories: all_inc,
  )
