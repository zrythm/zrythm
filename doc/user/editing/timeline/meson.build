copy_files = [
  'arranger.rst',
  'intro.rst',
  'overview.rst',
  'ruler.rst',
  'toolbar.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach

