# Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

if (not help2man.found ()) or os_windows
  subdir_done ()
endif

manpage_h2m = configure_file (
  output: 'zrythm.h2m',
  input: 'zrythm.h2m.in',
  configuration: {
    'COPYRIGHT_NAME': copyright_name,
    'COPYRIGHT_YEARS': copyright_years,
    }
  )

manpage = custom_target (
  'manpage',
  input: manpage_h2m,
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

# gen completions
bash_completion = custom_target (
  'bash-completion',
  output: 'zrythm',
  input: manpage,
  command: [
    gen_completions_sh, manpage.full_path (), 'bash',
    '@OUTPUT@',
    ],
  install: get_option ('completions'),
  install_dir: datadir / 'bash-completion/completions',
  build_by_default: get_option ('completions'))
zsh_completion = custom_target (
  'zsh-completion',
  output: '_zrythm',
  input: manpage,
  command: [
    gen_completions_sh, manpage.full_path (), 'zsh',
    '@OUTPUT@',
    ],
  install: get_option ('completions'),
  install_dir: datadir / 'zsh/site-functions',
  build_by_default: get_option ('completions'))
fish_completion = custom_target (
  'fish-completion',
  output: 'zrythm.fish',
  input: manpage,
  command: [
    gen_completions_sh, manpage.full_path (), 'fish',
    '@OUTPUT@',
    ],
  install: get_option ('completions'),
  install_dir: datadir / 'fish/vendor_completions.d',
  build_by_default: get_option ('completions'))

run_target (
  'build-manpage',
  command: [
    'echo',
    'Manpage built',
    ],
  depends: manpage,
  )
