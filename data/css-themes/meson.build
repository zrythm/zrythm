# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
