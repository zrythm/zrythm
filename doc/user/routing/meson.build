copy_files = [
  'inserts-sends.rst',
  'intro.rst',
  'making-connections.rst',
  'managing-connections.rst',
  'monitor-section.rst',
  'ports.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
