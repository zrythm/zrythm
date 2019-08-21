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
  version: '0.6.038',
  license: 'AGPLv3+',
  #meson_version: '>= 0.24.0',
  default_options: [
    'warning_level=1',
    'buildtype=debug',
    'c_std=gnu11' ],
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
locales = ['en', 'de', 'fr', 'ja']

cdata = configuration_data ()
cdata.set_quoted (
  'PACKAGE_VERSION',
  meson.project_version ())
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
cdata.set (
  'MESON_SOURCE_ROOT',
  meson.source_root ())
cdata.set (
  'MESON_BUILD_ROOT',
  meson.build_root ())
cdata.set_quoted (
  'ISSUE_TRACKER_URL',
  'https://savannah.nongnu.org/support/?group=zrythm')
cdata.set_quoted (
  'NEW_ISSUE_URL',
  'https://savannah.nongnu.org/support/?func=additem&group=zrythm')
cdata.set (
  'HAVE_MANUAL',
  get_option ('user_manual') ? 1 : 0)

os_darwin = false
os_linux = false
os_freebsd = false

if host_machine.system() == 'darwin'
  os_darwin = true
elif host_machine.system() == 'linux'
  os_linux = true
elif host_machine.system() == 'freebsd'
  os_freebsd = true
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
all_inc = [
  root_inc,
  inc_inc,
  ext_inc,
  midilib_inc,
  cyaml_inc,
  zix_inc,
  ]

resources_dir = join_paths (
  meson.source_root (), 'resources')
data_dir = join_paths (
  meson.source_root (), 'data')

cc = meson.get_compiler ('c')

# Check for programs
find_program ('sed', required: true)
sphinx_build = find_program (
  ['sphinx-build', 'sphinx-build-3'],
  required: get_option ('user_manual'))
help2man = find_program (
  ['help2man'],
  required: get_option ('manpage'))

check_headers = [
  'unistd.h',
  ]

foreach h : check_headers
  if cc.has_header(h)
    cdata.set('HAVE_' + h.underscorify().to_upper(), 1)
  endif
endforeach

# Maths functions might be implemented in libm
libm = cc.find_library('m', required: false)

check_functions = [
  'mlock',
  'jack_set_property',
  ]
foreach func : check_functions
  if cc.has_function(func, dependencies: libm)
    cdata.set('HAVE_' + func.underscorify().to_upper(), 1)
  endif
endforeach

# Compiler flags
if cc.get_id() == 'msvc'
  # Compiler options taken from msvc_recommended_pragmas.h
  # in GLib, based on _Win32_Programming_ by Rector and Newcomer
  test_cflags = []
  add_project_arguments('-FImsvc_recommended_pragmas.h', language: [ 'c', 'cpp' ])
  add_project_arguments('-D_USE_MATH_DEFINES', language: [ 'c', 'cpp' ])
elif cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  test_cflags = [
  '-Wall',
	'-Wno-missing-field-initializers',
	'-Wno-unused-parameter',
	'-Wno-sequence-point',
	'-Wignored-qualifiers',
  '-Wno-cast-function-type',
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
  ]
else
  test_cflags = []
endif

if get_option ('enable_profiling')
  test_cflags += [ '-g', '-pg', 'no-pie' ]
endif

test_cflags += [
  '-fPIC',
  ]

common_cflags = []
foreach f : test_cflags
  if cc.has_argument (f)
    common_cflags += f
  endif
endforeach

test_ldflags = []

if get_option ('enable_profiling')
  test_ldflags += [ '-g', '-pg', 'no-pie' ]
endif

if os_linux
  test_ldflags += [
    '-lasound',
    ]
endif

if not os_darwin
  test_ldflags += [
    '-lm',
    ]
endif

if os_freebsd
  test_ldflags += [
    '-lexecinfo'
    ]
endif

test_ldflags += [
  '-lfftw3_threads',
  '-lfftw3f_threads'
  ]

if (meson.version() >= '0.46.0')
  common_ldflags = cc.get_supported_link_arguments (
    test_ldflags)
else
  common_ldflags = test_ldflags
endif

# set config defines
libgtop_dep = dependency('libgtop-2.0', required: false)
if (libgtop_dep.found ())
  cdata.set('HAVE_LIBGTOP', 1)
endif
x11_dep = dependency('x11', required: false)
if (x11_dep.found ())
  cdata.set('HAVE_X11', 1)
endif
jack_dep = dependency ('jack', required: false)
if (jack_dep.found ())
  cdata.set('HAVE_JACK', 1)
endif
port_audio_dep = dependency (
  'portaudio-2.0', required: false)
if (port_audio_dep.found ())
  cdata.set('HAVE_PORT_AUDIO', 1)
endif
qt5_dep = dependency('Qt5Widgets', required: false)

yaml_dep = dependency('yaml-0.1')
gtk_dep = dependency('gtk+-3.0', version: '>=3.20')
sndfile_dep = dependency('sndfile', version: '>=1.0.25')

zrythm_deps = [
  jack_dep,
  port_audio_dep,
  yaml_dep,
  gtk_dep,
  sndfile_dep,
  libgtop_dep,
  dependency('threads'),
  dependency('lilv-0', version: '>=0.24.2'),
  dependency('samplerate', version: '>=0.1.8'),
  dependency('lv2', version: '>=1.14.0'),
  dependency('fftw3', version: '>=3.3.5'),
  libm,
]

if (qt5_dep.found() and get_option('enable_qt5'))
  cdata.set('HAVE_QT5', 1)
  zrythm_deps += qt5_dep
endif

if os_linux
  zrythm_deps += cc.find_library('rt')
endif

if os_linux or os_freebsd
  zrythm_deps += x11_dep
endif

if (os_darwin)
  zrythm_deps += dependency (
    'appleframeworks',
    modules: [
      'foundation',
      'cocoa',
      'appkit',
      ])
endif

configure_file (
  output: 'config.h',
  configuration: cdata,
  )

test_cflags_c_only = [
  '-Wno-bad-function-cast',
  '-Werror=absolute-value',
  '-Werror=parentheses-equality',
  ]

common_cflags_c_only = []
foreach f : test_cflags_c_only
  if cc.has_argument (f)
    common_cflags_c_only += f
  endif
endforeach

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
  join_paths ('scripts', 'meson_post_install.py'))

ext_srcs = []

subdir ('po')
subdir ('ext')
subdir ('src')
subdir ('tests')
subdir ('data')
subdir ('resources')
subdir ('doc')

summary = [
  '',
  '------',
  'Zrythm @0@'.format(meson.project_version()),
  '',
  '  Profiling: @0@'.format(
    get_option('enable_profiling')),
  '  Build Tests: @0@'.format(get_option('enable_tests')),
  '  Generate developer docs: @0@'.format(get_option('gen_dev_docs')),
  '  Build/install manpage: @0@'.format(get_option('manpage')),
  '  Build/install user manual: @0@'.format(get_option('user_manual')),
  '  Coverage reports: @0@'.format(get_option('b_coverage')),
  '  Optional libraries:',
  '    ffmpeg: @0@'.format(get_option('enable_ffmpeg')),
  '    Qt5: @0@'.format(get_option('enable_qt5') and qt5_dep.found()),
  '    Jack: @0@'.format(jack_dep.found()),
  '    PortAudio: @0@'.format(port_audio_dep.found()),
  '    Gtop: @0@'.format(libgtop_dep.found()),
  'Directories:',
  '            prefix: @0@'.format(prefix),
  '        includedir: @0@'.format(includedir),
  '            libdir: @0@'.format(libdir),
  '           datadir: @0@'.format(datadir),
  '------',
  ''
]

message('\n'.join(summary))
