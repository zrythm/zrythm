custom_css_target = custom_target (
  'custom-css',
  input: 'custom_css.scss',
  output: 'custom.css',
  command: [
    sass, '@INPUT@', '@OUTPUT@',
    ]
  )

run_target (
  'manual_compile_css',
  command : [
    'echo', 'Compiled custom CSS',
    ],
  depends: custom_css_target)
