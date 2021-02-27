copy_files = [
  'audio-editor.rst',
  'automation-editor.rst',
  'chord-editor.rst',
  'intro.rst',
  'overview.rst',
  'piano-roll.rst',
  'ruler.rst',
  'toolbar.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
