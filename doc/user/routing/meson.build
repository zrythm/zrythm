copy_files = [
  'control-room.rst',
  'inserts-sends.rst',
  'intro.rst',
  'making-connections.rst',
  'ports.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
