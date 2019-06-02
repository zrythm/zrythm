cyaml_inc = [
  include_directories ('include'),
  ]
cyaml = static_library (
  'cyaml',
  sources: [
    'src' / 'mem.c',
    'src' / 'save.c',
    'src' / 'free.c',
    'src' / 'load.c',
    'src' / 'utf8.c',
    'src' / 'util.c',
    ],
  dependencies: yaml_dep,
  include_directories: cyaml_inc,
)
