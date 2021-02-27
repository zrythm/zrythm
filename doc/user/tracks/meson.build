copy_files = [
  'audio-track.rst',
  'bus-track-audio.rst',
  'chord-track.rst',
  'group-track-audio.rst',
  'inspector-page.rst',
  'intro.rst',
  'master-track.rst',
  'modulator-track.rst',
  'tempo-track.rst',
  'track-types.rst',
  'automation.rst',
  'bus-track-midi.rst',
  'creating-tracks.rst',
  'group-track-midi.rst',
  'instrument-track.rst',
  'marker-track.rst',
  'midi-track.rst',
  'overview.rst',
  'track-operations.rst',
  'track-visibility.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
