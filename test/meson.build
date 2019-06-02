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

tests = {
  'audio/automation_track': { },
  'audio/track': { },
  }


foreach test_name, extra_args : tests
  source = extra_args.get('source', test_name + '.c')
  test_name = ' '.join(test_name.split('/'))

  exe = executable (
    test_name, source,
    c_args : test_cflags +
      extra_args.get('c_args', []),
    dependencies : [
      zrythm_deps,
      extra_args.get('dependencies', []),
      ],
    export_dynamic :
      extra_args.get('export_dynamic', false),
    link_with: zrythm_lib,
    include_directories : all_inc +
      extra_args.get('include_directories', []),
  )

  suite = ['zrythm'] + extra_args.get('suite', [])
  test (test_name, exe, env: test_env, suite: suite)
endforeach

endif
