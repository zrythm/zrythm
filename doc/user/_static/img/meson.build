c = run_command ([
  'find', '.', '-name', '*.png',
  ])
png_files = c.stdout().strip().split('\n')

foreach f : png_files
  configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
