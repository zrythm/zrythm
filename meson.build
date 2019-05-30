project (
  'zrythm', ['c', 'cpp'],
  version: '0.4.186',
  license: 'GPLv3+',
  meson_version: '>= 0.43.0',
  default_options: [
    'warning_level=1',
    'buildtype=debugoptimized',
    'c_std=gnu11' ],
  )

prefix = get_option ('prefix')
bindir = prefix / get_option('bindir')
libdir = prefix / get_option ('libdir')
libexecdir = prefix / get_option ('libexecdir')
includedir = prefix / get_option('includedir')
datadir = prefix / get_option('datadir')
sysconfdir = prefix / get_option('sysconfdir')
schemasdir = datadir / 'glib-2.0/schemas'

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

root_inc = include_directories ('.')
inc_inc = include_directories ('inc')
ext_inc = include_directories ('ext')
midilib_inc = include_directories ('ext' / 'midilib')
cyaml_inc = include_directories ('ext' / 'libcyaml')
zix_inc = include_directories ('ext' / 'zix')
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

common_cflags = cc.get_supported_arguments(test_cflags)

common_ldflags = []

if cc.get_id() == 'gcc'
  common_ldflags += [
    '-rdynamic',
    ]
elif cc.get_id() == 'clang'
  common_ldflags += [
    '-Wl,--export-dynamic',
    ]
endif

if os_win32
  common_ldflags += [
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
  common_ldflags += [
    '-lrt',
    '-lasound',
    ]
endif

if not os_win32 and not os_darwin
  common_ldflags += [
    '-lm',
    ]
endif

if os_freebsd
  common_ldflags += [
    '-lexecinfo'
    ]
endif

if get_option ('enable_profiling')
  common_ldflags += '-pg'
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

zrythm_deps = [
  jack_dep,
  port_audio_dep,
  yaml_dep,
  dependency('threads'),
  dependency('gtk+-3.0', version: '>=3.20'),
  dependency('lilv-0', version: '>=0.24.2'),
  dependency('sndfile', version: '>=1.0.25'),
  dependency('samplerate', version: '>=0.1.8'),
  dependency('lv2', version: '>=1.14.0'),
  dependency('Qt5Widgets', required: false),
  cc.find_library('m', required: false),
  cc.find_library('rt', required: false),
]

configure_file (
  output: 'config.h',
  configuration: cdata,
  )

add_project_arguments (
  common_cflags,
  language: 'c'
  )

meson.add_install_script (
  'scripts' / 'meson_post_install.py')

gnome = import('gnome')

ext_srcs = []

subdir ('po')
subdir ('ext')
subdir ('src')
subdir ('test')
subdir ('data')
subdir ('resources')
