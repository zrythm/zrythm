copy_files = [
  'intro.rst',
  'chord-pad.rst',
  'piano-roll-integration.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
