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

  subdir('eg-amp.lv2')
  subdir('eg-fifths.lv2')

  test_env = environment ()
  test_env.set (
    'G_TEST_SRC_ROOT_DIR', meson.source_root ())
  test_env.set (
    'G_TEST_SRCDIR', meson.current_source_dir ())
  test_env.set (
    'G_TEST_BUILDDIR', meson.current_build_dir ())
  test_env.set ('G_DEBUG', 'gc-friendly')
  test_env.set ('MALLOC_CHECK', '2')
  test_env.set ('VST_PATH', '/tmp/zrythm_vst')
  test_env.set ('LV2_PATH', '/tmp/zrythm_lv2')

  test_cflags = [
    '-DTESTS_SRCDIR="' +
      meson.current_source_dir () +
      '"',
    '-DTESTS_BUILDDIR="' +
      meson.current_build_dir () +
      '"',
  ]

  test_link_args = [
    '-fPIC',
  ]

  # 0: path
  # 1: is parallel
  tests = [
    ['actions/arranger_selections', true],
    ['audio/automation_track', true],
    ['audio/curve', true],
    ['audio/midi_mapping', true],
    ['audio/midi_note', true],
    ['audio/midi_track', true],
    ['audio/position', true],
    ['audio/region', true],
    ['audio/track', true],
    ['project', true],
    ['utils/arrays', true],
    ['utils/general', true],
    ['utils/string', true],
    ]

  if os_linux
    tests += [
      ['actions/mixer_selections', true],
      ]
  endif

  if get_option ('enable_gui_tests')
    tests += [
      ['gui/widgets/region', false],
      ['gui/widgets/track', false],
      ]
  endif

  if (chromaprint_dep.found ())
    tests += [
      ['audio/exporter', true],
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
        test_cflags,
        ],
      dependencies : [
        zrythm_deps,
        ],
      export_dynamic : false,
      link_with: zrythm_lib,
      include_directories : all_inc,
    )

    test (
      test_name, exe,
      env: test_env, suite: _test[0].split('/')[0],
      is_parallel: parallel)
  endforeach

endif
