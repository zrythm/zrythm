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

if help2man.found() and not os_windows
manpage = custom_target (
  'manpage',
  input: 'zrythm.h2m',
  output: 'zrythm.1',
  depends: zrythm_exe,
  command: [
    help2man,
    '-s',
    '1',
    '-N',
    '-i',
    '@INPUT@',
    '-o',
    '@OUTPUT@',
    '-n',
    'Zrythm',
    '--version-string=Zrythm ' + meson.project_version (),
    zrythm_exe,
    ],
  install: get_option ('manpage'),
  install_dir: mandir,
  build_by_default: get_option ('manpage'),
  )

run_target (
  'build-manpage',
  command: [
    'echo',
    'Manpage built',
    ],
  depends: manpage,
  )
endif
