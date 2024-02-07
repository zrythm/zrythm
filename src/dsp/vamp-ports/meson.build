# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
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

vamp_sdk_dep = dependency (
  'vamp-sdk', static: vamp_static)

vamp_plugins_lib = static_library (
  'vamp-plugins',
  'FixedTempoEstimator.cpp',
  'BeatTrack.cpp',
  pic: true,
  link_with: [
    qm_dsp_lib,
    ],
  include_directories: [
    include_directories ('.'),
    qm_dsp_inc,
    ],
  dependencies: vamp_sdk_dep,
  )
