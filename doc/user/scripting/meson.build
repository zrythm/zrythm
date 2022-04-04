# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

script_contents = ''
foreach script : data_script_filenames
  path = meson_src_root / 'data/scripts' / script
  content = fs.read (path)
  content_lines = content.split ('\n')
  title = content_lines[0].substring (4)
  script_contents += title + '\n'
  script_contents += '-------------------------------------------------------------------------------'
  script_contents += '\n\n'
  script_contents += '.. code-block:: scheme\n\n'
  foreach line : content.split('\n')
    script_contents += '  ' + line + '\n'
  endforeach
  script_contents += '\n'
endforeach

scripting_examples_rst = configure_file (
  output: 'examples.rst',
  input: 'examples.rst.in',
  configuration: { 'SCRIPTS': script_contents })

copy_files = [
  'intro.rst',
  'overview.rst',
  'scripting-interface.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach

subdir ('api')
