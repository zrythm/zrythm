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

  tests = [
    'audio/automation_track',
    'audio/midi_note',
    'audio/region',
    'audio/track',
    ]


  foreach test_name : tests
    source = test_name + '.c'
    test_name = 'zrythm:' + ' '.join(test_name.split('/'))

    exe = executable (
      test_name, source,
      c_args : common_cflags,
      dependencies : [
        zrythm_deps,
        ],
      export_dynamic : false,
      link_with: zrythm_lib,
      include_directories : all_inc,
    )

    suite = ['zrythm']
    test (test_name, exe, env: test_env, suite: suite)
  endforeach

endif
