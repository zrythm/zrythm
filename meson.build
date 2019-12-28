# Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
  version: '0.7.295',
  license: 'AGPLv3+',
  meson_version: '>= 0.50.0',
  default_options: [
    'warning_level=1',
    'buildtype=debug',
    'c_std=gnu11',
    ],
  )

gnome = import('gnome')

prefix = get_option ('prefix')
bindir = join_paths (prefix, get_option('bindir'))
libdir = join_paths (prefix, get_option ('libdir'))
lv2dir = join_paths (libdir, 'lv2')
lv2zpluginsdir = join_paths (lv2dir, 'ZPlugins')
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
  'en', 'de', 'fr', 'ja', 'pt', 'pt_BR',
  'nb_NO']

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
  'CONFIGURE_DATADIR',
  datadir)
cdata.set_quoted (
  'CONFIGURE_LIBDIR',
  libdir)
cdata.set_quoted (
  'SAMPLES_DIR', samplesdir)
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
  cdata.set_quoted (
    'MANUAL_PATH', docdir)
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
  add_languages('objc')
endif

root_inc = include_directories ('.')
inc_inc = include_directories ('inc')
ext_inc = include_directories ('ext')
midilib_inc = include_directories (
  join_paths ('ext', 'midilib'))
cyaml_inc = include_directories (
  join_paths ('ext', 'libcyaml', 'include'))
zix_inc = include_directories (
  join_paths ('ext', 'zix'))
suil_inc = include_directories (
  join_paths ('inc', 'plugins', 'lv2'))
all_inc = [
  root_inc,
  inc_inc,
  ext_inc,
  midilib_inc,
  cyaml_inc,
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
  cdata.set_quoted('OPEN_DIR_CMD', open_dir_cmd)
elif os_darwin
  open_dir_cmd = 'open'
  find_program (open_dir_cmd)
  cdata.set_quoted('OPEN_DIR_CMD', open_dir_cmd)
elif os_windows
  open_dir_cmd = 'explorer'
  find_program (open_dir_cmd)
  cdata.set_quoted('OPEN_DIR_CMD', open_dir_cmd)
endif

sphinx_build = find_program (
  ['sphinx-build', 'sphinx-build-3'],
  required: get_option ('user_manual'))
help2man = find_program (
  ['help2man'],
  required: get_option ('manpage'))
stoat = find_program (
  ['stoat'], required: false)

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
if (jack_dep.found ())
  cdata.set('HAVE_JACK', 1)
  check_functions += [
    ['jack_set_property',[jack_dep]],
    ['jack_client_stop_thread', [jack_dep]],
    ]
endif

foreach func : check_functions
  if cc.has_function(func[0], dependencies: func[1])
    cdata.set('HAVE_' + func[0].underscorify().to_upper(), 1)
  endif
endforeach

# Compiler flags
test_cflags = [
  '-Wall',
  '-Wformat=2',
  '-Wno-missing-field-initializers',
  '-Wno-unused-parameter',
  '-Wno-sequence-point',
  '-Wignored-qualifiers',
  '-Wno-cast-function-type',
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

if not os_darwin
  test_ldflags += [
    '-lm',
    ]
endif

common_ldflags = cc.get_supported_link_arguments (
  test_ldflags)

if os_freebsd
  common_ldflags += [
    '-L' + libdir,
    '-lexecinfo',
    ]
endif

# set config defines
libgtop_dep = dependency('libgtop-2.0', required: false)
if (libgtop_dep.found ())
  cdata.set('HAVE_LIBGTOP', 1)
  cdata.set('TIME_WITH_SYS_TIME', 0)
endif
x11_dep = dependency('x11', required: false)
if (x11_dep.found ())
  cdata.set('HAVE_X11', 1)
endif
alsa_dep = dependency('alsa', required: false)
if (alsa_dep.found ())
  cdata.set('HAVE_ALSA', 1)
endif
cyaml_dep = dependency(
  'libcyaml', required: false, version: '>=0.2')
if (cyaml_dep.found ())
  cdata.set('HAVE_CYAML', 1)
endif
audec_dep = dependency(
  'audec', version: '>=0.1',
  fallback: ['libaudec', 'libaudec_dep'])

qt5_dep = dependency('Qt5Widgets', required: get_option('enable_qt5'))
yaml_dep = dependency('yaml-0.1')
gtk_dep = dependency('gtk+-3.0', version: '>=3.24')
sndfile_dep = dependency('sndfile', version: '>=1.0.25')
lv2_dep = dependency('lv2', version: '>=1.14.0')
fftw3_dep = dependency('fftw3', version: '>=3.3.5')

# TODO add Cantarell font as dependency

zrythm_deps = [
  jack_dep,
  yaml_dep,
  gtk_dep,
  sndfile_dep,
  libgtop_dep,
  alsa_dep,
  cyaml_dep,
  audec_dep,
  x11_dep,
  dependency('threads'),
  dependency('lilv-0', version: '>=0.24.2'),
  dependency('samplerate', version: '>=0.1.8'),
  dependency('rubberband'),
  lv2_dep,
  fftw3_dep,
  qt5_dep,

  # these are needed for gentoo
  dependency('fftw3_threads', required: false),
  dependency('fftw3f_threads', required: false),

  libm,
]

if (get_option('enable_ffmpeg'))
  zrythm_deps += dependency('libavcodec')
  zrythm_deps += dependency('libavformat')
  zrythm_deps += dependency('libavutil')
  cdata.set('HAVE_FFMPEG', 1)
endif

if (get_option('enable_qt5'))
  cdata.set('HAVE_QT5', 1)
  zrythm_deps += qt5_dep
endif

if (get_option('enable_portaudio'))
  zrythm_deps += dependency ('portaudio-2.0')
  cdata.set('HAVE_PORT_AUDIO', 1)
endif

if os_linux
  zrythm_deps += cc.find_library('rt')
endif

if os_linux or os_freebsd
  zrythm_deps += cc.find_library('fftw3_threads')
  zrythm_deps += cc.find_library('fftw3f_threads')
endif

if (os_darwin)
  zrythm_deps += cc.find_library('fftw3_threads')
  zrythm_deps += cc.find_library('fftw3f_threads')
  zrythm_deps += dependency (
    'appleframeworks',
    modules: [
      'foundation',
      'cocoa',
      'appkit',
      ])
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

meson.add_install_script (
  join_paths ('scripts', 'meson_post_install.py'),
  '@0@'.format(get_option('user_manual')))

ext_srcs = []

subdir ('po')
subdir ('ext')
subdir ('resources')
subdir ('src')
subdir ('plugins')
subdir ('tests')
subdir ('data')
subdir ('doc')
subdir ('scripts')

summary = [
  '',
  '------',
  'Zrythm @0@'.format(meson.project_version()),
  '',
  '  Build type: @0@'.format(
    get_option('buildtype')),
  '  Profiling: @0@'.format(
    get_option('enable_profiling')),
  '  Strict flags: @0@'.format(get_option('strict_flags')),
  '  Build plugins: @0@'.format(get_option('enable_plugins')),
  '  Build tests: @0@'.format(get_option('enable_tests')),
  '  Build GUI tests: @0@'.format(get_option('enable_gui_tests')),
  '  Generate developer docs: @0@'.format(get_option('gen_dev_docs')),
  '  Build/install manpage: @0@'.format(get_option('manpage')),
  '  Build/install user manual: @0@'.format(get_option('user_manual')),
  '  Install DSEG font: @0@'.format(get_option('install_dseg_font')),
  '  Coverage reports: @0@'.format(get_option('b_coverage')),
  '  Optional libraries:',
  '    ffmpeg: @0@'.format(get_option('enable_ffmpeg')),
  '    Jack (not really optional at the moment): @0@'.format(jack_dep.found()),
  '    Qt5: @0@'.format(get_option('enable_qt5')),
  '    PortAudio (not functional at the moment): @0@'.format(get_option('enable_portaudio')),
  '    Gtop: @0@'.format(libgtop_dep.found()),
  '  Directories:',
  '            prefix: @0@'.format(prefix),
  '        includedir: @0@'.format(includedir),
  '            libdir: @0@'.format(libdir),
  '           datadir: @0@'.format(datadir),
  '------',
  ''
]

message('\n'.join(summary))
