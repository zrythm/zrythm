copy_files = [
  'environment.rst',
  'files-and-directories.rst',
  'gnu-free-documentation-license.rst',
  'intro.rst',
  'menu-actions.rst',
  'meson.build',
  'shortcut-keys.rst',
  'troubleshooting.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
