# SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if get_option ('tests')

  test_lv2_plugins = []
  test_lv2_plugin_libs = []
  subdir('lv2plugins')

  pipewire_bin = find_program ('pipewire', required: false)

  test_env = environment ()
  test_env.set (
    'G_TEST_SRC_ROOT_DIR', meson_src_root)
  test_env.set (
    'G_TEST_SRCDIR', meson.current_source_dir ())
  test_env.set (
    'G_TEST_BUILD_ROOT_DIR', meson_build_root)
  test_env.set (
    'G_TEST_BUILDDIR', meson.current_build_dir ())
  test_env.set ('G_SLICE', 'debug-blocks')
  test_env.set (
    'GUILE_LOAD_PATH',
    '$GUILE_LOAD_PATH:' + meson.current_build_dir ())
  test_env.set ('G_DEBUG', 'gc-friendly')
  test_env.set ('MALLOC_CHECK_', '3')
  test_env.set ('VST_PATH', '/tmp/zrythm_vst')
  test_env.set ('VST3_PATH', '/tmp/zrythm_vst3')
  test_env.set ('LADSPA_PATH', '/tmp/zrythm_ladspa')
  test_env.set ('DSSI_PATH', '/tmp/zrythm_dssi')
  test_env.set ('CLAP_PATH', '/tmp/zrythm_clap')
  test_env.set ('ZRYTHM_DEBUG', '1')
  test_env.set ('Z_CURL_TIMEOUT', '10')
  test_env.set ('G_MESSAGES_DEBUG', 'zrythm')
  test_env.set ('ZRYTHM_DSP_THREADS', '4')
  test_env.set ('PIPEWIRE_RUNTIME_DIR', '/tmp/zrythm-pipewire')
  test_env.set ('PIPEWIRE_DEBUG', '3')

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
  test_config.set_quoted (
    'PIPEWIRE_SERVER_NAME', 'zrythm-pipewire-0')
  test_config.set ('HAVE_PIPEWIRE', pipewire_bin.found ())
  test_config.set_quoted ('PIPEWIRE_BIN', pipewire_bin.full_path ())
  test_config.set_quoted (
    'PIPEWIRE_CONF_PATH',
    meson.current_source_dir () / 'pipewire-server.conf')

  test_c_args = [
    '-DDEBUG_THREADS=1',
    ]

  test_config_guile = configuration_data()
  test_config_guile.merge_from (test_config)

  # FIXME use kuribu to generate the audio files
  # instead of shipping them in the source code:
  # ./bin/kuribu /path/to/midifile.mid out.wav http://moddevices.com/plugins/mod-devel/arpeggiator http://distrho.sf.net/plugins/Kars

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

  # generate 30-min audio file with sine wave
  if sox.found ()
    # timestretched test file x 2
    filename = 'test_sine_ogg_30min.ogg'
    test_sine_ogg_30min = configure_file (
      output: filename,
      command: [
        sox, '-n', '-r', '44100', '@OUTPUT@', 'synth', '1800',
        'sine', '500',
        ])
    test_config.set_quoted (
      'TEST_SINE_OGG_30MIN',
      meson.current_build_dir () / filename)
  endif

  # internal (bundled) LV2
  if not lv2info_bin.found ()
    error ('Missing lv2info required for tests')
  endif
  foreach pl : bundled_plugins_for_tests
    name = pl['name']
    uri = pl['uri']
    target = pl['target']
    plugin_bundle_uri = 'file://' + target.full_path() + '/'
    # C
    test_config.set (
      'HAVE_' + name.to_upper(), 1)
    test_config.set_quoted (
      name.to_upper() + '_URI', uri)
    test_config.set_quoted (
      name.to_upper() + '_BUNDLE', plugin_bundle_uri)
    # Guile
    test_config_guile.set (
      'HAVE_' + name.to_upper(), '#t')
    test_config_guile.set_quoted (
      name.to_upper() + '_URI', uri)
    test_config_guile.set_quoted (
      name.to_upper() + '_BUNDLE', plugin_bundle_uri)
  endforeach

  # external LV2
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

  # also add bundle/uri macros for test plugins
  foreach pl : test_lv2_plugins
    name = pl['name']
    uri = pl['uri']
    bundle = pl['bundle']
    lib = pl['lib']
    name_underscorified = name.underscorify ().to_upper ()
    # C
    test_config.set_quoted (
      name_underscorified + '_URI', uri)
    test_config.set_quoted (
      name_underscorified + '_BUNDLE_URI',
      'file://' + bundle + '/')
    # Guile
    test_config_guile.set_quoted (
      name_underscorified + '_URI', uri)
    test_config_guile.set_quoted (
      name_underscorified + '_BUNDLE_URI',
      'file://' + bundle + '/')
  endforeach

  subdir('presets')

  test_lv2apply_wavs = []
  if test_config.has ('HAVE_MVERB')
    file_prefix = 'test_mixdown_midi_routed_to_instrument_track'

    # add reverb to file
    test_lv2apply_wavs += custom_target (
      file_prefix + '_w_reverb.ogg',
      input: file_prefix + '.ogg',
      output: file_prefix + '_w_reverb.ogg',
      command: [
        lv2apply, '-i', '@INPUT@',
        '-o', '@OUTPUT@',
        test_config.get_unquoted ('MVERB_URI'),
        ])
    test_config.set_quoted (
      'TEST_WAV3',
      meson.current_build_dir () / file_prefix + '_w_reverb.ogg')

    # add reverb to file + half gain
    test_lv2apply_wavs += custom_target (
      file_prefix + '_w_reverb_half_gain.ogg',
      input: file_prefix + '.ogg',
      output: file_prefix + '_w_reverb_half_gain.ogg',
      command: [
        lv2apply, '-i', '@INPUT@',
        '-c', 'gain', '50',
        '-o', '@OUTPUT@',
        test_config.get_unquoted ('MVERB_URI'),
        ])
    test_config.set_quoted (
      'TEST_WAV4',
      meson.current_build_dir () / file_prefix + '_w_reverb_half_gain.ogg')
  endif

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

  # VST3
  foreach name, filename : ext_vst3_plugins
    have_plugin = have_ext_vst3_plugins.get (
      name, false)
    if have_plugin
      path = ext_vst3_plugin_paths[name]
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
  # 1: dictionary of args
  tests = {
    'actions/arranger_selections': {
      'parallel': false },
    'actions/channel_send': { 'parallel': true },
    'actions/range': { 'parallel': true },
    'actions/transport_action': { 'parallel': true },
    # skip CI because it uses too much RAM
    'actions/undo_manager': {
      'parallel': false,
      'extra_suites': [ 'skip-ci' ] },
    'dsp/audio_region': { 'parallel': true },
    'dsp/audio_track': { 'parallel': true },
    'dsp/automation_track': { 'parallel': true },
    'dsp/channel': { 'parallel': true },
    'dsp/chord_track': { 'parallel': true },
    'dsp/curve': { 'parallel': true },
    'dsp/fader': { 'parallel': true },
    'dsp/graph_export': { 'parallel': true },
    'dsp/marker_track': { 'parallel': true },
    'dsp/metronome': { 'parallel': true },
    'dsp/midi_event': { 'parallel': true },
    'dsp/midi_mapping': { 'parallel': true },
    'dsp/midi_note': { 'parallel': true },
    'dsp/midi_region': { 'parallel': false },
    'dsp/midi_track': { 'parallel': true },
    'dsp/pool': { 'parallel': false },
    'dsp/position': { 'parallel': true },
    'dsp/port': { 'parallel': true },
    'dsp/region': { 'parallel': true },
    'dsp/sample_processor': { 'parallel': true },
    'dsp/scale': { 'parallel': true },
    'dsp/snap_grid': { 'parallel': true },
    'dsp/tempo_track': { 'parallel': true },
    'dsp/track': { 'parallel': true },
    'dsp/track_processor': { 'parallel': true },
    'dsp/tracklist': { 'parallel': true },
    'dsp/transport': { 'parallel': true },
    'gui/backend/arranger_selections': {
      'parallel': true },
    'integration/memory_allocation': { 'parallel': true },
    'integration/recording': { 'parallel': false },
    'plugins/carla_discovery': { 'parallel': true },
    'plugins/carla_native_plugin': { 'parallel': false },
    'plugins/lv2_plugin': { 'parallel': false },
    'plugins/lv2/lv2_state': { 'parallel': false },
    'plugins/plugin': { 'parallel': false },
    'plugins/plugin_manager': { 'parallel': true },
    'project': { 'parallel': false },
    'settings/settings': { 'parallel': true },
    'utils/arrays': { 'parallel': true },
    'utils/compression': { 'parallel': true },
    'utils/file': { 'parallel': true },
    'utils/general': { 'parallel': true },
    'utils/hash': { 'parallel': true },
    'utils/math': { 'parallel': true },
    'utils/midi': { 'parallel': true },
    'utils/io': { 'parallel': true },
    'utils/string': { 'parallel': true },
    'utils/ui': { 'parallel': true },
    'utils/yaml': { 'parallel': true },
    'zrythm_app': { 'parallel': true },
    'zrythm': { 'parallel': true },
    }

  if os_gnu or os_darwin
    tests += {
      'actions/mixer_selections_action': {
        'parallel': false },
      'actions/port_connection': {
        'parallel': false },
      'actions/tracklist_selections': {
        'parallel': false },
      'actions/tracklist_selections_edit': {
        'parallel': false },
      'benchmarks/dsp': {
        'parallel': true,
        'benchmark': true, },
      'integration/midi_file': {
        'parallel': false },
      # cannot be parallel because it needs multiple
      # threads
      'integration/run_graph_with_latencies': {
        'parallel': false },
      'integration/undo_redo_helm_track_creation': {
        'parallel': false },
      'utils/system': {
        'parallel': false },
      }
  endif

  if get_option ('gui_tests')
    tests += {
      'gui/widgets/region': {
        'parallel': false },
      'gui/widgets/track': {
        'parallel': false },
      }
  endif

  if chromaprint_dep.found ()
    tests += {
      'dsp/exporter': {
        'parallel': true },
      }
  endif

  if have_guile
    foreach f : data_script_filenames
      tests += {
        ('guile/script_runner/' + f.substring (0, -4)).underscorify(): {
          'parallel': true,
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
          'parallel': true },
        }
    endif
  endif

  test_link_libs = []
  if os_windows
    # tests fail to link otherwise
    test_link_libs += zrythm_lib.get_static_lib ()
  else
    test_link_libs += zrythm_lib.get_shared_lib ()
  endif

  if have_guile
    test_link_libs += guilelib
  endif

  foreach name, info : tests
    is_parallel = info['parallel']
    test_name = '_'.join(name.split('/'))
    suites = [ name.split('/')[0] ]

    is_benchmark = false
    if 'benchmark' in info and info['benchmark'] == true
      is_benchmark = true
    endif

    if 'extra_suites' in info
      suites += info['extra_suites']
    endif
    if 'different_source' in info
      source = info['different_source']
    else
      source = name + '.c'
    endif
    timeout = 180
    foreach suite : suites
      if suite.contains ('benchmark') or suite.contains ('integration') or suite.contains ('actions')
        is_parallel = false
        timeout = 680
      endif
    endforeach

    if fs.is_file (source)
      # create C executable for C runner
      exe = executable (
        test_name,
        sources: [
          source,
          test_config_h,
          bundled_plugins_for_tests_tgts,
          ],
        c_args : [
          common_cflags, strict_cflags,
          test_c_args,
          ],
        dependencies: [
          zrythm_deps,
          ],
        export_dynamic: false,
        link_with: test_link_libs,
        include_directories: all_inc,
        #override_options: [ 'b_lto=false', ],
      )
      if is_benchmark
        benchmark (
          test_name, exe,
          env: test_env, suite: suites,
          args: 'args' in info ? info['args'] : [],
          depends: [
            test_lv2apply_wavs,
            test_lv2_plugin_libs,
            ],
          timeout: timeout)
      # else if not benchmark
      else
        test (
          test_name, exe,
          env: test_env, suite: suites,
          is_parallel: is_parallel,
          args: 'args' in info ? info['args'] : [],
          depends: [
            test_lv2apply_wavs,
            test_lv2_plugin_libs,
            ],
          timeout: timeout)
      endif # endif is_benchmark
    else
      # create C executable for Guile runner
      source = _test[0] + '.scm'
      exe = executable (
        test_name,
        sources: [
          'guile_runner.c',
          test_config_h,
          bundled_plugins_for_tests_tgts,
          ],
        c_args : [
          common_cflags, strict_cflags,
          test_c_args,
          ],
        dependencies : [
          zrythm_deps,
          ],
        export_dynamic : false,
        link_with: test_link_libs,
        include_directories : all_inc,
        )

      if is_benchmark
        benchmark (
          test_name, exe,
          args: source,
          env: test_env, suite: suites,
          timeout: timeout)
      else
        test (
          test_name, exe,
          args: source,
          env: test_env, suite: suites,
          is_parallel: is_parallel,
          timeout: timeout)
      endif
    endif
  endforeach

  add_test_setup (
    'ci',
    exclude_suites: 'skip-ci',
    )

endif
