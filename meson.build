# Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
#
# This file is part of Zrythm
#
# Zrythm is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Zrythm is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.

project (
  'zrythm', ['c', 'cpp'],
  version: '0.8.200',
  license: 'AGPL-3-or-later',
  meson_version: '>= 0.50.0',
  default_options: [
    'warning_level=2',
    'buildtype=debug',
    'c_std=gnu11',
    ],
  )

gnome = import('gnome')

prefix = get_option ('prefix')
bindir = join_paths (prefix, get_option('bindir'))
libdir = join_paths (prefix, get_option ('libdir'))
libexecdir = join_paths (prefix, get_option ('libexecdir'))
includedir = join_paths (prefix, get_option('includedir'))
datadir = join_paths (prefix, get_option('datadir'))
sysconfdir = join_paths (prefix, get_option('sysconfdir'))
mandir = join_paths (datadir, 'man', 'man1')
schemasdir = join_paths (datadir, 'glib-2.0/schemas')
fontsdir = join_paths (datadir, 'fonts', 'zrythm')
zrythmdatadir = join_paths (datadir, 'zrythm')
samplesdir = join_paths (zrythmdatadir, 'samples')
docdir = join_paths (datadir, 'doc', 'zrythm')

# Used for building manpages, manual, etc., using
# foreach
locales = [
  'en', 'en_GB', 'gl', 'de', 'fr', 'it',
  'nb_NO', 'es', 'ja',
  'pl', 'pt', 'pt_BR',
  'ru', 'zh' ]

cdata = configuration_data ()
cdata.set_quoted (
  'PACKAGE_VERSION',
  '@VCS_TAG@')
cdata.set_quoted (
  'COMPILER',
  meson.get_compiler('c').get_id())
cdata.set_quoted (
  'PREFIX', prefix)
cdata.set_quoted (
  'COMPILER_VERSION',
  meson.get_compiler('c').version())
cdata.set_quoted (
  'CONFIGURE_DATADIR', datadir)
cdata.set_quoted (
  'LIBDIR_NAME', get_option ('libdir'))
cdata.set_quoted (
  'CONFIGURE_LIBDIR', libdir)
if get_option ('trial_ver')
  cdata.set('TRIAL_VER', 1)
endif
if get_option ('windows_release')
  cdata.set ('WINDOWS_RELEASE', 1)
endif
if get_option ('mac_release')
  cdata.set ('MAC_RELEASE', 1)
endif
cdata.set_quoted('SAMPLES_DIR', samplesdir)
cdata.set (
  'MESON_SOURCE_ROOT',
  meson.source_root ())
cdata.set (
  'MESON_BUILD_ROOT',
  meson.build_root ())
cdata.set_quoted (
  'ISSUE_TRACKER_URL',
  'https://redmine.zrythm.org/projects/zrythm/issues')
cdata.set_quoted (
  'NEW_ISSUE_URL',
  'https://redmine.zrythm.org/projects/zrythm/issues/new')
if (get_option ('user_manual'))
  cdata.set_quoted ('MANUAL_PATH', docdir)
endif

os_darwin = false
os_linux = false
os_freebsd = false
os_windows = false

if host_machine.system() == 'darwin'
  os_darwin = true
elif host_machine.system() == 'linux'
  os_linux = true
elif host_machine.system() == 'freebsd'
  os_freebsd = true
elif host_machine.system() == 'windows'
  os_windows = true
endif

if os_darwin
  add_languages(['objc', 'objcpp'])
elif os_windows
  cdata.set ('USE_MMCSS_THREAD_PRIORITIES', 1)
endif

root_inc = include_directories ('.')
inc_inc = include_directories ('inc')
ext_inc = include_directories ('ext')
midilib_inc = include_directories (
  join_paths ('ext', 'midilib'))
carla_native_plugin_inc = include_directories (
  join_paths ('ext', 'carla'))
zix_inc = include_directories (
  join_paths ('ext', 'zix'))
suil_inc = include_directories (
  join_paths ('inc', 'plugins', 'lv2'))
all_inc = [
  root_inc,
  inc_inc,
  ext_inc,
  carla_native_plugin_inc,
  midilib_inc,
  zix_inc,
  suil_inc,
  ]

resources_dir = join_paths (
  meson.source_root (), 'resources')
data_dir = join_paths (
  meson.source_root (), 'data')

cc = meson.get_compiler ('c')

# --- Check for programs ---

find_program ('sed', required: true)

# find command to open a directory
if os_linux or os_freebsd
  open_dir_cmd = 'xdg-open'
  find_program (open_dir_cmd)
elif os_darwin
  open_dir_cmd = 'open'
  find_program (open_dir_cmd)
elif os_windows
  open_dir_cmd = 'explorer.exe'
endif
cdata.set_quoted('OPEN_DIR_CMD', open_dir_cmd)

sphinx_build = find_program (
  ['sphinx-build', 'sphinx-build-3'],
  required: get_option ('user_manual'))
help2man = find_program (
  ['help2man'],
  required: get_option ('manpage'))
stoat = find_program (
  ['stoat'], required: false)
python3 = find_program (
  ['python3', 'python'])
guile = find_program (
  ['guile2.2', 'guile'])
guild = find_program (
  ['guild2.2', 'guild'], required: false)
guile_snarf = find_program (
  ['guile-snarf2.2', 'guile-snarf'], required: false)
pandoc = find_program (
  ['pandoc'], required: false)
texi2html = find_program (
  ['texi2html'], required: false)
scripts_conf = {
  'SCRIPTS_DIR': meson.source_root () / 'scripts',
  'MESON_BUILD_ROOT': meson.build_root (),
  }
if os_windows
  scripts_conf += { 'GUILE': 'guile' }
else
  scripts_conf += { 'GUILE': guile.path() }
endif
if pandoc.found()
  scripts_conf += { 'PANDOC_BIN': pandoc.path() }
endif
if texi2html.found()
  scripts_conf += {
    'TEXI2HTML_BIN': texi2html.path() }
endif
if guile_snarf.found()
  scripts_conf += {
    'GUILE_SNARF': guile_snarf.path() }
  guile_snarf_wrap_path = 'scripts' / 'guile-snarf-wrap.scm'
  guile_snarf_wrap = configure_file (
    output: 'guile-snarf-wrap.scm',
    input: guile_snarf_wrap_path,
    configuration: scripts_conf,
    )
endif
if get_option('guile_snarf_docs_path') != ''
  scripts_conf += {
    'GUILE_SNARF_DOCS_BIN': get_option('guile_snarf_docs_path'),
    'GUILD_BIN': guild.path() }
  guile_gen_docs_path = 'scripts' / 'guile-gen-docs.scm'
  guile_gen_docs = configure_file (
    output: 'guile-gen-docs.scm',
    input: guile_gen_docs_path,
    configuration: scripts_conf,
    )
endif
meson_post_install_path = 'scripts' / 'meson-post-install.scm'
meson_post_install = configure_file (
  output: 'meson-post-install.scm',
  input: meson_post_install_path,
  configuration: scripts_conf,
  )
post_install_wrap_path = 'scripts' / 'meson_post_install_wrap.sh'
post_install_wrap = configure_file (
  output: 'meson_post_install_wrap.sh',
  input: post_install_wrap_path,
  configuration: scripts_conf,
  )

check_headers = [
  'unistd.h',
  'sys/time.h',
  ]

foreach h : check_headers
  if cc.has_header(h)
    cdata.set('HAVE_' + h.underscorify().to_upper(), 1)
  endif
endforeach

# Maths functions might be implemented in libm
libm = cc.find_library('m', required: false)

check_functions = [
  ['mlock',libm],
  ]

# prefer jack1
jack_dep = dependency (
  'jack', required: false, version: '<1.0')
if (not jack_dep.found ())
  jack_dep = dependency ('jack', required: false)
endif
if get_option ('enable_jack') and jack_dep.found ()
  cdata.set('HAVE_JACK', 1)
  check_functions += [
    ['jack_set_property',[jack_dep]],
    ['jack_client_stop_thread', [jack_dep]],
    ]
elif get_option ('enable_jack')
  error ('libjack not found. Please install it or set \'enable_jack\' to false')
endif

foreach func : check_functions
  if cc.has_function(func[0], dependencies: func[1])
    cdata.set('HAVE_' + func[0].underscorify().to_upper(), 1)
  endif
endforeach

# Compiler flags
test_cflags = [
  '-Wformat=2',
  '-Wno-missing-field-initializers',
  '-Wno-unused-parameter',
  '-Wno-sequence-point',
  '-Wignored-qualifiers',
  '-Wno-cast-function-type',
  '-fno-common',
  ]

if (get_option ('buildtype') == 'release')
  test_cflags += [
    '-ffast-math',
    '-DPIC',
    '-fdata-sections',
    '-ffunction-sections',
    '-mtune=generic',
    '-msse',
    '-msse2',
    '-mfpmath=sse',
    #'-fvisibility=hidden',
    ]
  #cdata.set('G_DISABLE_CHECKS', 1)
endif

if get_option ('enable_profiling')
  test_cflags += [ '-g', '-pg', 'no-pie' ]
endif

common_cflags = cc.get_supported_arguments (
  test_cflags)

if os_freebsd
  common_cflags += [
    '-I' + includedir,
    ]
endif

if os_windows
  common_cflags += [
    '-mms-bitfields',
    '-mwindows',
    '-Wl,-Bdynamic',
    '-Wl,-as-needed',
    '-D_WOE32=1',
    ]
endif

test_strict_cflags = [
  #'-Werror=cast-qual',
  '-Werror=clobbered',
  #'-Werror=conversion',
  '-Werror=disabled-optimization',
  '-Werror=double-promotion',
  '-Werror=float-equal',
  '-Werror=logical-op',
  '-Werror=pointer-arith',
  '-Werror=sign-conversion',
  '-Werror=overlength-strings',
  '-Werror=stringop-truncation',
  '-Werror=missing-declarations',
  '-Werror=int-to-pointer-cast',
  #'-Werror=redundant-decls',
  '-Werror=shadow',
  '-Werror=undef',
  '-Werror=unused',
  '-Werror=strict-aliasing',
  '-fstrict-aliasing',
  #'-Werror=strict-overflow',
  '-Wstrict-overflow=2',
  '-fstrict-overflow',
  '-Werror=duplicated-branches',
  '-Werror=duplicated-cond',
  '-Werror=null-dereference',
  '-Werror=init-self',
  '-Werror=jump-misses-init',
  '-Werror=missing-prototypes',
  '-Werror=nested-externs',
  '-Werror=write-strings',
  '-Werror=implicit-fallthrough',
  '-Werror=sign-compare',
  '-Werror=discarded-qualifiers',
  '-Werror=float-conversion',
  '-Werror=implicit-function-declaration',
  '-Werror=uninitialized',
  '-Werror=maybe-uninitialized',
  '-Werror=return-type',
  '-Werror=int-conversion',
  '-Werror=format-security',
  '-Werror=incompatible-pointer-types',
  '-Werror=implicit-int',
  '-Werror=multistatement-macros',
  '-Werror=switch',
  '-Werror=overflow',
  '-Werror=array-bounds',
  '-Werror=enum-compare',
  '-Werror=misleading-indentation',
  '-Werror=int-in-bool-context',
  '-Werror=type-limits',
  '-Werror=deprecated-declarations',
  '-Werror=format-extra-args',
  '-Werror=format',
  ]

if cc.get_id() == 'gcc'
  test_strict_cflags += [
    '-Wextra',
    '-Weverything',
    ]
endif

strict_cflags = []
if get_option ('strict_flags')
  strict_cflags = cc.get_supported_arguments (
    test_strict_cflags)
endif

# add -Wformat -Werror=format-security
if cc.get_id() == 'gcc'
  common_cflags += [
    '-Wformat',
    '-Werror=format-security']
endif

test_ldflags = []

if get_option ('enable_profiling')
  test_ldflags += [ '-g', '-pg', 'no-pie' ]
endif

common_ldflags = cc.get_supported_link_arguments (
  test_ldflags)

if os_freebsd
  common_ldflags += [
    '-L' + libdir,
    '-lexecinfo',
    ]
endif

if os_windows
  common_ldflags += [
    '-fno-stack-protector',
    '-lws2_32',
    '-static-libgcc',
    '-static-libstdc++',
    ]
endif

# set config defines
libgtop_dep = dependency(
  'libgtop-2.0', required: false)
if (libgtop_dep.found ())
  cdata.set('HAVE_LIBGTOP', 1)
  cdata.set('TIME_WITH_SYS_TIME', 0)
endif
if not os_darwin
  x11_dep = dependency('x11', required: false)
  if (x11_dep.found ())
    cdata.set('HAVE_X11', 1)
  endif
endif
alsa_dep = dependency('alsa', required: false)
if (alsa_dep.found ())
  # disable as it's not working
  # cdata.set('HAVE_ALSA', 1)
endif
cyaml_dep = dependency(
  'libcyaml', required: false, version: '=1.0.1')
if not cyaml_dep.found()
  cyaml_proj = subproject('libcyaml')
  cyaml_dep = cyaml_proj.get_variable(
    'zrythm_cyaml_dep')
endif
audec_dep = dependency(
  'audec', version: '>=0.1',
  fallback: ['libaudec', 'libaudec_dep'],
  required: true)

gtk_dep = dependency('gtk+-3.0', version: '>=3.22')
sndfile_dep = dependency('sndfile', version: '>=1.0.25')
lv2_dep = dependency(
  'lv2', version: '>=1.16.0', required: false)
if not lv2_dep.found()
  lilv_proj = subproject('lilv')
  lv2_dep = lilv_proj.get_variable('lv2_dep')
endif
lilv_dep = dependency (
  'lilv-0', version: '>=0.24.6',
  fallback: ['lilv', 'lilv_dep'])
fftw3_dep = dependency('fftw3', version: '>=3.3.5')

chromaprint_dep = dependency (
  'libchromaprint', required: false)
if (chromaprint_dep.found ())
  cdata.set('HAVE_CHROMAPRINT', 1)
endif

# TODO add Cantarell font as dependency

zrythm_deps = [
  jack_dep,
  gtk_dep,
  sndfile_dep,
  libgtop_dep,
  alsa_dep,
  cyaml_dep,
  audec_dep,
  dependency('threads'),
  lilv_dep,
  dependency(
    'sratom-0',
    fallback: ['lilv', 'sratom_dep']),
  dependency(
    'serd-0',
    fallback: ['lilv', 'serd_dep']),
  dependency(
    'sord-0',
    fallback: ['lilv', 'sord_dep']),
  dependency('samplerate', version: '>=0.1.8'),
  dependency('rubberband'),
  dependency ('libpcre'),
  cc.find_library('dl'),
  lv2_dep,
  chromaprint_dep,
  fftw3_dep,

  # these are needed for gentoo
  dependency('fftw3_threads', required: false),
  dependency('fftw3f_threads', required: false),

  libm,
]

if (get_option('enable_carla'))
  cdata.set('HAVE_CARLA', 1)
  carla_native_plugin_dep = dependency(
    'carla-native-plugin')
  carla_utils_dep = dependency('carla-utils')
  # workaround because carla's pc files are broken
  carla_includedir = carla_native_plugin_dep.get_pkgconfig_variable('includedir')
  carla_libdir = carla_native_plugin_dep.get_pkgconfig_variable('libdir')
  carla_native_plugin_dep = declare_dependency(
    link_args: [
      '-Wl,-rpath=' + carla_libdir,
      '-L' + carla_libdir,
      '-lcarla_native-plugin',
      ],
    compile_args: [
      '-DREAL_BUILD',
      '-I' + carla_includedir,
      '-I' + join_paths (carla_includedir, 'includes'),
      ])
  # workaround because carla's pc files are broken
  carla_includedir = carla_utils_dep.get_pkgconfig_variable('includedir')
  carla_libdir = carla_utils_dep.get_pkgconfig_variable('libdir')
  carla_utils_dep = declare_dependency(
    link_args: [
      '-Wl,-rpath=' + carla_libdir,
      '-L' + carla_libdir,
      '-lcarla_utils',
      ],
    compile_args: [
      '-DREAL_BUILD',
      '-I' + carla_includedir,
      '-I' + join_paths (carla_includedir, 'includes'),
      '-I' + join_paths (carla_includedir, 'utils'),
      ])
  zrythm_deps += [
    carla_native_plugin_dep,
    carla_utils_dep, ]
endif

if (get_option('enable_ffmpeg'))
  zrythm_deps += dependency('libavcodec')
  zrythm_deps += dependency('libavformat')
  zrythm_deps += dependency('libavutil')
  cdata.set('HAVE_FFMPEG', 1)
endif

if (get_option('enable_portaudio'))
  zrythm_deps += dependency ('portaudio-2.0')
  # disable until it works
  #cdata.set('HAVE_PORT_AUDIO', 1)
endif

if (get_option('enable_rtmidi'))
  rtmidi_dep = dependency ('rtmidi', required: false)
  if not rtmidi_dep.found()
    rtmidi_proj = subproject('rtmidi')
    rtmidi_dep = rtmidi_proj.get_variable(
      'zrythm_rtmidi_dep')
  endif
  zrythm_deps += rtmidi_dep
  cdata.set('HAVE_RTMIDI', 1)
endif

if (get_option('enable_rtaudio'))
  rtaudio_dep = dependency (
    'rtaudio', required: false)
  if not rtaudio_dep.found()
    rtaudio_proj = subproject('rtaudio')
    rtaudio_dep = rtaudio_proj.get_variable(
      'zrythm_rtaudio_dep')
  endif
  zrythm_deps += rtaudio_dep
  cdata.set('HAVE_RTAUDIO', 1)
endif

if (get_option('enable_sdl'))
  zrythm_deps += dependency ('sdl2')
  cdata.set('HAVE_SDL', 1)
endif

if get_option('enable_guile')
  guile_dep = dependency (
    'guile-3.0', required: false)
  if not guile_dep.found ()
    guile_dep = dependency ('guile-2.2')
  endif
  zrythm_deps += guile_dep
#zrythm_deps += dependency ('gtksourceview-3.0')
  cdata.set('HAVE_GUILE', 1)
#cdata.set('HAVE_GTK_SOURCE_VIEW', 1)
endif

if os_windows
  zrythm_deps += cc.find_library('dbghelp')
  # msys2 provides a separate fftw3f entry in
  # pkg-config
  zrythm_deps += dependency('fftw3f')
else
  zrythm_deps += cc.find_library('fftw3_threads')
  zrythm_deps += cc.find_library('fftw3f_threads')
endif

if os_linux
  zrythm_deps += cc.find_library('rt')
endif

if (os_darwin)
  zrythm_deps += dependency (
    'appleframeworks',
    modules: [
      'foundation',
      'cocoa',
      'appkit',
      ])
else
  zrythm_deps += cc.find_library('m')
  zrythm_deps += x11_dep
endif

# create config.h
tmp_h = configure_file (
  output: 'tmp.h',
  configuration: cdata,
  )
config_h_vcs = vcs_tag (
  input: tmp_h,
  output: 'config.h',
  )
config_h_dep = declare_dependency (
  sources: config_h_vcs,
  )
zrythm_deps += config_h_dep

test_cflags_c_only = [
  '-Wno-bad-function-cast',
  '-Wno-old-style-declaration',
  '-Werror=absolute-value',
  '-Werror=parentheses-equality',
  ]

common_cflags_c_only = cc.get_supported_arguments (
  test_cflags_c_only)

add_project_arguments (
  common_cflags_c_only,
  language: [ 'c' ]
  )
add_project_arguments (
  common_cflags,
  language: [ 'c', 'cpp' ],
  )
add_project_link_arguments (
  common_ldflags,
  language: [ 'c', 'cpp' ],
  )

ext_srcs = []

subdir ('po')
subdir ('ext')
subdir ('resources')
subdir ('data')
subdir ('src')
subdir ('tests')
subdir ('doc')
subdir ('scripts')

meson.add_install_script (
  meson.build_root() / 'meson_post_install_wrap.sh')

if meson.version().version_compare('>=0.53')
summary({
  'Build type': get_option('buildtype'),
  'Profiling': get_option('enable_profiling'),
  'Strict flags': get_option('strict_flags'),
  'Build tests': get_option('enable_tests'),
  'Build GUI tests': get_option('enable_gui_tests'),
  'Generate developer docs': get_option('gen_dev_docs'),
  'Build/install manpage': get_option('manpage'),
  'Build/install user manual': get_option('user_manual'),
  'Install DSEG font': get_option('install_dseg_font'),
  'Coverage reports': get_option('b_coverage'),
  }, section: 'General')

summary({
  'FFmpeg': get_option('enable_ffmpeg'),
  'Jack': get_option('enable_jack'),
  'PortAudio': get_option('enable_portaudio'),
  'RtMidi': get_option('enable_rtmidi'),
  'RtAudio': get_option('enable_rtaudio'),
  'SDL': get_option('enable_sdl'),
  'Carla': get_option('enable_carla'),
  'Gtop': libgtop_dep.found(),
  'Guile': get_option('enable_guile'),
  }, section: 'Optional libraries')

summary({
  'prefix': prefix,
  'includedir': includedir,
  'libdir': libdir,
  'datadir': datadir,
  }, section: 'Directories')
endif
