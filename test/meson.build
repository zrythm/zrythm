if get_option ('enable_tests')

test_env = [
  'G_TEST_SRCDIR=@0@'.format (
    meson.current_source_dir()),
  'G_TEST_BUILDDIR=@0@'.format (
    meson.current_build_dir()),
  'G_DEBUG=gc-friendly',
  'GSETTINGS_BACKEND=memory',
  'PYTHONDONTWRITEBYTECODE=yes',
  'MALLOC_CHECK_=2',
]

test_cflags = [
  '-fPIE',
  '-DTEST_DATA_DIR="@0@/data"'.format (
    meson.current_source_dir()),
]

test_link_args = [
  '-fPIC',
]

#test_application = executable (
  #'test-application', [zrythm_srcs, 'automations.c', 'main.c'],
  #c_args: test_cflags,
  #link_args: test_link_args,
  #link_with: cyaml,
  #dependencies: zrythm_deps,
  #include_directories: all_inc,
#)

#test (
  #'test-application',
  #test_application,
  #env: test_env)

endif
