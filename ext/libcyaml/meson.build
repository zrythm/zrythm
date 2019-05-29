cyaml_inc = include_directories ('include')
cyaml = static_library (
  'cyaml',
  sources: [
    join_paths ('src', 'mem.c'),
    join_paths ('src', 'save.c'),
    join_paths ('src', 'free.c'),
    join_paths ('src', 'load.c'),
    join_paths ('src', 'utf8.c'),
    join_paths ('src', 'util.c'),
    ],
  dependencies: yaml_dep,
  include_directories: cyaml_inc,
)
