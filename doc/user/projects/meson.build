copy_files = [
  'intro.rst',
  'project-management-and-compatibility.rst',
  'project-structure.rst',
  'saving-loading.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
