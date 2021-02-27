copy_files = [
  'css.rst',
  'icon-themes.rst',
  'intro.rst',
  'overview.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
