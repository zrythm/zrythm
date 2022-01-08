# Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

if os_windows
  zrythm_rc = configure_file (
    output: 'zrythm.rc',
    input: 'zrythm.rc.in',
    configuration: {
      'ZRYTHM_MAJOR_VER': 1,
      'ZRYTHM_MINOR_VER': 0,
      'ZRYTHM_PATCH_VER': 0,
      'ZRYTHM_MICRO_VER': 0,
      'COPYRIGHT_NAME': copyright_name,
      'COPYRIGHT_YEARS': copyright_years,
      'PRODUCT_VERSION': meson.project_version(),
      })

  zrythm_ico = configure_file (
    output: 'zrythm.ico',
    input: 'zrythm.ico',
    copy: true)

  winmod = import('windows')
  win_res = winmod.compile_resources (
    zrythm_rc,
    depend_files: 'zrythm.ico')
endif
