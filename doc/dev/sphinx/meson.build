# SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if sphinx_build.found ()
  # note: needs exhale to be inside the ext dir
  sphinx_dev_target = custom_target (
    'sphinx-dev-docs',
    depend_files: [
      doxyfile,
      zrythm_srcs,
      extra_dev_docs,
      ],
    input: [
      'source/conf.py',
      'source/index.rst',
      ],
    output: 'sphinx-dev-docs-built',
    command: [
      sphinx_build, '-b', 'html',
      '-D', 'breathe_projects.Zrythm=' + meson.current_build_dir () / '..' / 'xml/',
      '-j', '4',
      meson.current_source_dir () / 'source',
      meson.current_build_dir () / 'build', '&&',
      'touch', '@OUTPUT@',
      ],
    install: false,
    build_by_default: false,
    depends: mcss_docs_target,
    )

  run_target (
    'gen-sphinx-dev-docs',
    command: [
      'echo',
      'Developer Docs generated (sphinx)',
      ],
    depends: sphinx_dev_target,
    )
endif
