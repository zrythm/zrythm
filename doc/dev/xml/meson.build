doxygen = find_program('doxygen')
c_index_xml = custom_target (
  'c-index.xml',
  command: [doxygen, '@INPUT0@'],
  input: [doxyfile],
  output: 'index.xml')
