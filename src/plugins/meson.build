# SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
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

plugin_srcs = files ([
  'cached_plugin_descriptors.c',
  'carla_native_plugin.c',
  'collection.c',
  'collections.c',
  'lv2_plugin.c',
  'plugin.c',
  'plugin_descriptor.c',
  'plugin_gtk.c',
  'plugin_identifier.c',
  'plugin_manager.c',
  'plugin_preset.c',
  ])

subdir ('carla')
subdir ('lv2')

zrythm_srcs += plugin_srcs
