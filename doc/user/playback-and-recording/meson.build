copy_files = [
  'bpm-and-time-signatures.rst',
  'intro.rst',
  'loop-points-and-markers.rst',
  'overview.rst',
  'recording.rst',
  'transport-controls.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
