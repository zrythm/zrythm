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

if get_option ('enable_tests')

  test_env = environment ()
  test_env.set (
    'G_TEST_SRCDIR', meson.current_source_dir ())
  test_env.set (
    'G_TEST_BUILDDIR', meson.current_build_dir ())
  test_env.set ('G_DEBUG', 'gc-friendly')
  test_env.set ('MALLOC_CHECK', '2')

  test_link_args = [
    '-fPIC',
  ]

  # path, is parallel
  tests = [
    ['audio/automation_track', true],
    ['audio/midi_note', true],
    ['audio/midi_track', true],
    ['audio/position', true],
    ['audio/region', true],
    ['audio/track', true],
    ['utils/arrays', true],
    ]

  if get_option ('enable_gui_tests')
    tests += [
      ['gui/widgets/region', false],
      ['gui/widgets/track', false],
      ]
  endif

  foreach _test : tests
    test_name = _test[0]
    parallel = _test[1]
    source = [ test_name + '.c' ]
    test_name = '_'.join(test_name.split('/'))

    exe = executable (
      test_name,
      sources: [
        source,
        zrythm_resources,
        ],
      c_args : [
        common_cflags,
        ],
      dependencies : [
        zrythm_deps,
        ],
      export_dynamic : false,
      link_with: zrythm_lib,
      include_directories : all_inc,
    )

    suite = ['zrythm']
    test (
      test_name, exe,
      env: test_env, suite: suite,
      is_parallel: parallel)
  endforeach

endif
