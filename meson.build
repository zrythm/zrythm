project (
  'zrythm', ['c', 'cpp'],
  version: '0.5.024',
  license: 'GPLv3+',
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
schemasdir = join_paths (datadir, 'glib-2.0/schemas')

cdata = configuration_data ()
cdata.set_quoted (
  'PACKAGE_VERSION',
  meson.project_version ())
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

os_darwin = false
os_win32 = false
os_linux = false
os_freebsd = false

if host_machine.system() == 'darwin'
  os_darwin = true
elif host_machine.system() == 'windows'
  os_win32 = true
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

check_headers = [
  'unistd.h',
  ]
if (os_win32)
  check_headers += [
    'windows.h',
    'dlfcn.h',
    ]
endif

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
  add_project_arguments('-FImsvc_recommended_pragmas.h', language: 'c')
  add_project_arguments('-D_USE_MATH_DEFINES', language: 'c')
elif cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  test_cflags = [
  '-Wall',
	'-Wno-missing-field-initializers',
	'-Wno-unused-parameter',
	'-Wno-sequence-point',
	'-Wignored-qualifiers',
  '-Wno-bad-function-cast',
  '-Wno-cast-function-type',
  '-Werror=maybe-uninitialized',
	'-Werror=implicit-function-declaration',
	'-Werror=uninitialized',
	'-Werror=return-type',
	'-Werror=int-conversion',
	'-Werror=format-security',
	'-Werror=incompatible-pointer-types',
	'-Werror=implicit-int',
  '-Werror=multistatement-macros',
  '-Werror=switch',
  '-Werror=absolute-value',
  '-Werror=parentheses-equality',
  ]
else
  test_cflags = []
endif

if (os_win32)
  test_cflags += [
    '-mms-bitfields',
    '-mwindows',
    ]
else
  test_cflags += [
    '-fPIC',
    ]
endif

if get_option ('enable_profiling')
  test_cflags += '-pg'
endif

if get_option ('enable_coverage')
  test_cflags += [
    '-fprofile-arcs',
    '-ftest-coverage',
    '-g',
    '-O0',
    ]
endif

common_cflags = []
foreach f : test_cflags
  if cc.has_argument (f)
    common_cflags += f
  endif
endforeach

test_ldflags = []

if os_win32
  test_ldflags += [
    '-fno-stack-protector',
    '-lws2_32',
    '-static',
    '-static-libgcc',
    '-static-libstdc++',
    '-Wl,-Bdynamic',
    '-Wl,-as-needed',
    ]
endif

if os_linux
  test_ldflags += [
    '-lrt',
    '-lasound',
    ]
endif

if not os_win32 and not os_darwin
  test_ldflags += [
    '-lm',
    ]
endif

if os_freebsd
  test_ldflags += [
    '-lexecinfo'
    ]
endif

if get_option ('enable_profiling')
  test_ldflags += '-pg'
endif

if get_option ('enable_coverage')
  test_ldflags += [
    '-lgcov',
    '--coverage',
    ]
endif

if (meson.version() >= '0.46.0')
  common_ldflags = cc.get_supported_link_arguments (
    test_ldflags)
else
  common_ldflags = test_ldflags
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

yaml_dep = dependency('yaml-0.1')
gtk_dep = dependency('gtk+-3.0', version: '>=3.20')
sndfile_dep = dependency('sndfile', version: '>=1.0.25')

zrythm_deps = [
  jack_dep,
  port_audio_dep,
  yaml_dep,
  gtk_dep,
  sndfile_dep,
  dependency('threads'),
  dependency('lilv-0', version: '>=0.24.2'),
  dependency('samplerate', version: '>=0.1.8'),
  dependency('lv2', version: '>=1.14.0'),
  dependency('Qt5Widgets', required: false),
  libm,
  cc.find_library('rt', required: false),
]

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

add_project_arguments (
  common_cflags,
  language: 'c'
  )

add_project_link_arguments (
  common_ldflags,
  language: 'c',
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
  '  Run Profiling: @0@'.format(
    get_option('enable_profiling')),
  '  Build Tests: @0@'.format(get_option('enable_tests')),
  '  Generate Developer Docs: @0@'.format(get_option('gen_dev_docs')),
  '  Print Coverage (needs tests): @0@'.format(get_option('enable_coverage')),
  '  Optional libraries:',
  '    FFMPEG support: @0@'.format(get_option('enable_ffmpeg')),
  'Directories:',
  '            prefix: @0@'.format(prefix),
  '        includedir: @0@'.format(includedir),
  '            libdir: @0@'.format(libdir),
  '           datadir: @0@'.format(datadir),
  '------',
  ''
]

message('\n'.join(summary))
