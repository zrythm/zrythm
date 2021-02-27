copy_files = [
  'audio-and-midi.rst',
  'intro.rst',
  'routing-graph.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
