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

if get_option ('tests')

  subdir('eg-amp.lv2')
  subdir('eg-fifths.lv2')
  subdir('test-instrument.lv2')

  test_env = environment ()
  test_env.set (
    'G_TEST_SRC_ROOT_DIR', meson_src_root)
  test_env.set (
    'G_TEST_SRCDIR', meson.current_source_dir ())
  test_env.set (
    'G_TEST_BUILDDIR', meson.current_build_dir ())
  test_env.set ('G_SLICE', 'debug-blocks')
  test_env.set (
    'GUILE_LOAD_PATH', '$GUILE_LOAD_PATH:' + meson.current_build_dir ())
  test_env.set ('G_DEBUG', 'gc-friendly')
  test_env.set ('MALLOC_CHECK', '2')
  test_env.set ('VST_PATH', '/tmp/zrythm_vst')
  test_env.set ('VST3_PATH', '/tmp/zrythm_vst3')
  test_env.set ('LADSPA_PATH', '/tmp/zrythm_ladspa')
  test_env.set ('DSSI_PATH', '/tmp/zrythm_dssi')
  test_env.set ('ZRYTHM_DEBUG', '1')
  test_env.set ('G_MESSAGES_DEBUG', 'zrythm')

  test_config = configuration_data ()
  test_config.set_quoted (
    'TESTS_SRC_ROOT_DIR', meson_src_root)
  test_config.set_quoted (
    'TESTS_SRCDIR', meson.current_source_dir ())
  test_config.set_quoted (
    'TESTS_BUILDDIR', meson.current_build_dir ())
  test_config.set_quoted (
    'MIDILIB_TEST_MIDI_FILES_PATH',
    meson_src_root / 'ext/midilib/MIDIFiles')

  test_config_guile = configuration_data()
  test_config_guile.merge_from (test_config)

  if rubberband.found()
    # timestretched test file x 2
    test_wav2 = configure_file (
      input: 'test.wav',
      output: 'test2.wav',
      command: [
        rubberband, '--time', '2',
        '@INPUT@', '@OUTPUT@',
        ])
    test_config.set_quoted (
      'TEST_WAV2',
      meson.current_build_dir () / 'test2.wav')
  endif

  # LV2
  foreach name, info : ext_lv2_plugins
    uri = info[1]
    have_plugin = have_ext_lv2_plugins.get (
      name, false)
    if have_plugin
      plugin_bundle = ext_lv2_plugin_bundles[name]
      # C
      test_config.set (
        'HAVE_' + name.to_upper(), 1)
      test_config.set_quoted (
        name.to_upper() + '_URI', uri)
      test_config.set_quoted (
        name.to_upper() + '_BUNDLE', plugin_bundle)
      # Guile
      test_config_guile.set (
        'HAVE_' + name.to_upper(), '#t')
      test_config_guile.set_quoted (
        name.to_upper() + '_URI', uri)
      test_config_guile.set_quoted (
        name.to_upper() + '_BUNDLE', plugin_bundle)
    else
      # Guile
      test_config_guile.set (
        'HAVE_' + name.to_upper(), '#f')
      test_config_guile.set_quoted (
        name.to_upper() + '_URI', '')
      test_config_guile.set_quoted (
        name.to_upper() + '_BUNDLE', '')
    endif
  endforeach

  # VST
  foreach name, filename : ext_vst_plugins
    have_plugin = have_ext_vst_plugins.get (
      name, false)
    if have_plugin
      path = ext_vst_plugin_paths[name]
      # C
      test_config.set (
        'HAVE_' + name.to_upper(), 1)
      test_config.set_quoted (
        name.to_upper() + '_PATH', path)
      # Guile
      test_config_guile.set (
        'HAVE_' + name.to_upper(), '#t')
      test_config_guile.set_quoted (
        name.to_upper() + '_PATH', path)
    else
      # Guile
      test_config_guile.set (
        'HAVE_' + name.to_upper(), '#f')
      test_config_guile.set_quoted (
        name.to_upper() + '_PATH', '')
    endif
  endforeach
  test_config_h = configure_file (
    output: 'zrythm-test-config.h',
    configuration: test_config,
    )
  test_config_scm = configure_file (
    input: 'guile-config.scm.in',
    output: 'zrythm-test-config.scm',
    configuration: test_config_guile,
    )

  test_link_args = [
    '-fPIC',
  ]

  # 0: path
  # 1: is parallel
  # 2: args
  parallel = 'parallel'
  tests = {
    'actions/arranger_selections': {
      parallel: false },
    'actions/range': { parallel: true },
    'actions/undo_manager': { parallel: true },
    'audio/audio_region': { parallel: true },
    'audio/audio_track': { parallel: true },
    'audio/automation_track': { parallel: true },
    'audio/curve': { parallel: true },
    'audio/fader': { parallel: true },
    'audio/metronome': { parallel: true },
    'audio/midi': { parallel: true },
    'audio/midi_mapping': { parallel: true },
    'audio/midi_note': { parallel: true },
    'audio/midi_region': { parallel: true },
    'audio/midi_track': { parallel: true },
    'audio/position': { parallel: true },
    'audio/port': { parallel: true },
    'audio/region': { parallel: true },
    'audio/snap_grid': { parallel: true },
    'audio/track': { parallel: true },
    'audio/tracklist': { parallel: true },
    'gui/backend/arranger_selections': {
      parallel: true },
    'integration/recording': { parallel: false },
    'plugins/carla_discovery': { parallel: false },
    'plugins/plugin': { parallel: false },
    'plugins/plugin_manager': { parallel: true },
    'project': { parallel: true },
    'utils/arrays': { parallel: true },
    'utils/general': { parallel: true },
    'utils/io': { parallel: true },
    'utils/string': { parallel: true },
    'utils/ui': { parallel: true },
    'zrythm': { parallel: true },
    }

  if os_gnu or os_darwin
    tests += {
      'actions/mixer_selections_action': {
        parallel: false },
      'actions/port_connection': {
        parallel: true },
      'actions/tracklist_selections': {
        parallel: false },
      'actions/tracklist_selections_edit': {
        parallel: false },
      'benchmarks/dsp': {
        parallel: true },
      'integration/midi_file': {
        parallel: false },
      # cannot be parallel because it needs multiple
      # threads
      'integration/run_graph_with_latencies': {
        parallel: false },
      'integration/undo_redo_helm_track_creation': {
        parallel: false },
      'utils/system': {
        parallel: false },
      }
  endif

  if get_option ('gui_tests')
    tests += {
      'gui/widgets/region': {
        parallel: false },
      'gui/widgets/track': {
        parallel: false },
      }
  endif

  if chromaprint_dep.found ()
    tests += {
      'audio/exporter': {
        parallel: true },
      }
  endif

  if have_guile
    foreach f : data_script_filenames
      tests += {
        ('guile/script_runner/' + f.substring (0, -4)).underscorify(): {
          parallel: true,
          'args': meson_src_root / 'data/scripts' / f,
          'different_source': 'guile/script_runner.c',
          },
        }
    endforeach
    # disable for now until we have a way to load/
    # pass plugin bundles
    if false
      tests += {
        'guile/project_generator': {
          parallel: true },
        }
    endif
  endif

  test_link_libs = [ zrythm_lib.get_shared_lib () ]
  if have_guile
    test_link_libs += guilelib
  endif

  foreach name, info : tests
    is_parallel = info[parallel]
    test_name = '_'.join(name.split('/'))
    suite = name.split('/')[0]
    if 'different_source' in info
      source = info['different_source']
    else
      source = name + '.c'
    endif
    timeout = 180
    if suite.contains ('benchmark') or suite.contains ('integration') or suite.contains ('actions')
      is_parallel = false
      timeout = 360
    endif

    if fs.is_file (source)
      # create C executable for C runner
      exe = executable (
        test_name,
        sources: [
          source,
          test_config_h,
          ],
        c_args : [
          common_cflags, strict_cflags,
          ],
        dependencies : [
          zrythm_deps,
          ],
        export_dynamic : false,
        link_with: test_link_libs,
        include_directories : all_inc,
      )
      test (
        test_name, exe,
        env: test_env, suite: suite,
        is_parallel: is_parallel,
        args: 'args' in info ? info['args'] : [],
        timeout: timeout)
    else
      # create C executable for Guile runner
      source = _test[0] + '.scm'
      exe = executable (
        test_name,
        sources: [
          'guile_runner.c',
          test_config_h,
          ],
        c_args : [
          common_cflags, strict_cflags,
          ],
        dependencies : [
          zrythm_deps,
          ],
        export_dynamic : false,
        link_with: test_link_libs,
        include_directories : all_inc,
        )
      test (
        test_name, exe,
        args: source,
        env: test_env, suite: suite,
        is_parallel: is_parallel,
        timeout: timeout)
    endif
  endforeach

endif
