# SPDX-FileCopyrightText: © 2019-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

cdata_dev_docs = configuration_data ()
cdata_dev_docs.merge_from (cdata)
cdata_dev_docs.set ('DOXYGEN_AWESOME_CSS_PATH', meson.current_source_dir () / 'doxygen-awesome-css')
cdata_dev_docs.set ('MESON_DEVDOCS_BUILDDIR', meson.current_build_dir ())

doxyfile = configure_file (
  output: 'Doxyfile.cfg.novcs',
  input: 'Doxyfile.cfg.in',
  configuration: cdata_dev_docs,
  )
doxyfile = vcs_tag (
  output: 'Doxyfile.cfg',
  input: doxyfile,
  fallback: version_fallback,
  )

extra_dev_docs = files([
  meson_src_root / 'CONTRIBUTING.md',
  'mainpage.h',
  'cyaml_schemas.h',
  'gtk_tips.md',
  'processing_cycle.h',
  'release_checklist.h',
  'weblate.h',
  ])

if dot_bin.found() and doxygen.found()
  doxygen_docs = custom_target (
    'doxygen-docs',
    input: doxyfile,
    depend_files: [
      zrythm_srcs,
      extra_dev_docs,
      ],
    output: 'html',
    command: [
      doxygen, '@INPUT0@',
      ],
    install: false,
    build_by_default: false,
    )

  run_target (
    'gen-dev-docs',
    command: [
      'echo',
      'Developer Docs generated (Doxygen Awesome CSS)',
      ],
    depends: doxygen_docs,
    )
endif
