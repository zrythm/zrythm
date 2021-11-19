# Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

# Install precompiled .css if available or compile
# and install .scss
css_theme_names = [
  'zrythm', #'fruity',
  ]
foreach css_theme_name : css_theme_names
  css_theme_file = '@0@-theme.css'.format (css_theme_name)
  if fs.exists (css_theme_file)
    install_data (
      css_theme_file,
      install_dir: themes_css_dir,
      )
  else
    sassc = find_program ('sassc')
    css_theme_file = custom_target (
      '@0@-theme-css'.format (css_theme_name),
      input: '@0@/theme.scss'.format (css_theme_name),
      output: css_theme_file,
      command: [
        sassc, '--sass', '--omit-map-comment',
        '--style', 'nested',
        '@INPUT@', '@OUTPUT@',
        ],
      install: true,
      install_dir: themes_css_dir,
      )
  endif
endforeach
