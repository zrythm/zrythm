copy_files = [
  'common-operations.rst',
  'edit-tools.rst',
  'intro.rst',
  'overview.rst',
  'quantization.rst',
  'snapping-grid-options.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach

subdir ('clip-editors')
subdir ('timeline')
