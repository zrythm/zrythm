copy_files = [
  'connections.rst',
  'intro.rst',
  'monitor-section.rst',
  'ports.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
