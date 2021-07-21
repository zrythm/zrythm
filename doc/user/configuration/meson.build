copy_files = [
  'additional-settings.rst',
  'device-setup.rst',
  'intro.rst',
  'preferences.rst',
  'shortcuts.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
