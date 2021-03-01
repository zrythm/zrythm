copy_files = [
  'bottom-panel.rst',
  'intro.rst',
  'left-panel.rst',
  'main-toolbar.rst',
  'right-panel.rst',
  'timeline.rst',
  'transport-bar.rst',
  ]

foreach f : copy_files
  configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
