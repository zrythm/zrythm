copy_files = [
  'file-browser.rst',
  'intro.rst',
  'overview.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
