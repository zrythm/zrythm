# SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if not get_option ('bundled_plugins')
  subdir_done ()
endif

plugins = [
  [ 'Compressor',
    'Basic compressor',
    'CompressorPlugin' ],
  [ 'Zita Rev1',
    'Zita reverb algorithm',
    'ReverbPlugin' ],
  ]

gen_lv2_sh = configure_file (
  output: 'gen-lv2.sh',
  input: 'gen-lv2.sh.in',
  configuration: {
    'BASH': bash.full_path (),
    'URI_PREFIX': 'https://lv2.zrythm.org/faust-builtin',
    },
  )
gen_lv2_from_generated_sh = configure_file (
  output: 'gen-lv2-from-generated.sh',
  input: 'gen-lv2-from-generated.sh.in',
  configuration: {
    'BASH': bash.full_path (),
    },
  )

pl_generate_targets = []

foreach pl : plugins
  pl_name = pl[0]
  pl_underscored_name = pl_name.to_lower ().underscorify ()
  pl_descr = pl[1]
  pl_type = pl[2]

  if faust2lv2.found ()
    pl_dsp = configure_file (
      output: pl_underscored_name + '.dsp',
      input: pl_underscored_name + '.dsp.in',
      configuration: {
        'NAME': pl_name,
        'DESCRIPTION': pl_descr,
        'AUTHOR': 'Zrythm DAW',
        },
      )

    plugin = custom_target (
      pl_underscored_name + '-lv2-gen',
      input: [pl_dsp, 'zrythm-utils.lib'],
      output: pl_underscored_name + '.lv2.gen',
      command: [
        gen_lv2_sh, faust2lv2.full_path (),
        '@INPUT0@', '@PRIVATE_DIR@', '@OUTPUT@',
        pl_type, '@INPUT1@',
        meson.current_source_dir () / 'generated'],
      install: false)

    pl_generate_targets += plugin
  endif

  pl_cpp = 'generated' / pl_underscored_name + '.lv2' / pl_underscored_name + '.cpp'
  if fs.is_file (pl_cpp)
    pl_lib = shared_module (
      pl_underscored_name,
      cpp_args: [
        '-std=c++11', '-O3' ],
      sources: pl_cpp,
      name_prefix: '',
      gnu_symbol_visibility: 'hidden',
      dependencies: dependency ('boost'),
      override_options: [
        'b_sanitize=none',
        ]
      )

    plugin = custom_target (
      pl_underscored_name + '-lv2',
      input: [
        pl_lib,
        'generated' / pl_underscored_name + '.lv2' / 'manifest.ttl',
        'generated' / pl_underscored_name + '.lv2' / pl_underscored_name + '.ttl',
        ],
      output: pl_underscored_name + '.lv2',
      command: [
        gen_lv2_from_generated_sh,
        pl_underscored_name, '@INPUT0@',
        '@INPUT1@', '@INPUT2@', '@OUTPUT@',
        ],
      install_dir: zrythm_libdir / 'lv2',
      install: true)
  else
    warning (
      pl_cpp + ' not found. Run `ninja -C build gen-faust-plugins` to generate it')
  endif
endforeach

if faust2lv2.found ()
  run_target (
    'gen-faust-plugins',
    command: [
      'echo', 'Generated faust plugins', ],
    depends: pl_generate_targets)
endif
