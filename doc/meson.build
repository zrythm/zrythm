doxyfile = configure_file (
  output: 'Doxyfile.cfg',
  input: 'Doxyfile.cfg.in',
  configuration: cdata,
  install: false,
  )

doxyfile_mcss = configure_file (
  output: 'Doxyfile-mcss',
  input: 'Doxyfile-mcss.in',
  configuration: cdata,
  install: false,
  )

html_target = custom_target (
  'html-docs',
  depend_files: [
    doxyfile,
    doxyfile_mcss,
    zrythm_srcs,
    'mainpage.h',
    'cyaml_schemas.h',
    'processing_cycle.h',
    'release_checklist.h',
    'weblate.h',
    ],
  output: 'html',
  command: [
    join_paths (meson.current_source_dir (), 'm.css', 'documentation', 'doxygen.py'),
    doxyfile_mcss,
    ],
  install: false,
  )

run_target (
  'docs',
  command: [
    'echo',
    'Developer Docs generated',
    ],
  depends: html_target,
  )

run_target (
  'clean_docs',
  command: [
    'rm',
    '-rf',
    'html',
    ],
  )
