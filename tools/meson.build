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

if cppcheck.found () and jq.found ()

  # get files from compile_commands.json
  cppcheck_files = run_command (
    jq, '--raw-output', '.[].file',
    meson_build_root / 'compile_commands.json').stdout ().strip ().split ('\n')

  ignore_paths = [
    '../src/plugins/lv2',
    '../subprojects',
    '../ext',
    '/usr/include/guile/2.2/libguile',
    ]
  ignore_commands = []
  foreach ignore_path : ignore_paths
    ignore_commands += [ '-i' + ignore_path ]
  endforeach

  # Go through each file and generate a custom target.
  #
  # The full list of other files is passed as an
  # ignore list because we can't pass a single file
  # to cppcheck when using the --project option, and
  # we want to use the --project option because it
  # contains the build commands/defines/includes.
  #
  # The targets are split so that we can allow ninja
  # to use concurrency.
  cppcheck_targets = []
  processed_targets = []
  foreach cppcheck_file : cppcheck_files
    skip = false
    if cppcheck_file in processed_targets
      skip = true
    endif
    foreach ignore_path : ignore_paths
      if cppcheck_file.startswith (ignore_path)
        skip = true
        break
      endif
    endforeach
    if skip
      continue
    endif

    name = cppcheck_file.underscorify () + '.cppcheck'
    other_files = []
    foreach f : cppcheck_files
      if f != cppcheck_file
        other_files += '-i' + f
      endif
    endforeach
    cppcheck_targets += custom_target (
      name,
      output: name,
      command: [
        cppcheck,
        '--project=' + meson_build_root / 'compile_commands.json',
        '--include=' + meson_build_root / 'zrythm-config.h',
        '--template=gcc',
        '--error-exitcode=1',
        '-DGETTEXT_PACKAGE=zrythm',
        '-DDEPRECATED_MSG(x)=',
        ignore_commands,
        other_files,
        ])
    processed_targets += cppcheck_file
  endforeach

  run_target (
    'cppcheck',
    command : [
      'echo', 'done',
      ],
    depends: cppcheck_targets)
endif
