# Copyright (C) 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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

copy_files = [
  'bottom-panel.rst',
  'intro.rst',
  'left-panel.rst',
  'main-panel.rst',
  'main-toolbar.rst',
  'overview.rst',
  'right-panel.rst',
  'transport-bar.rst',
  ]

foreach f : copy_files
  configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
