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
  version: '0.8.694',
  license: 'AGPL-3-or-later',
  meson_version: '>= 0.53.0',
  default_options: [
    'warning_level=2',
    'buildtype=debugoptimized',
    'c_std=gnu11',
    ],
  )

gnome = import('gnome')
fs = import('fs')

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
themesdir = join_paths (zrythmdatadir, 'themes')
samplesdir = join_paths (zrythmdatadir, 'samples')
docdir = join_paths (datadir, 'doc', 'zrythm')
sourceviewstylesdir = join_paths (zrythmdatadir, 'sourceview-styles')

# Used for building manpages, manual, etc., using
# foreach
locales = run_command ('cat', 'po/LINGUAS').stdout ().strip ().split (' ') + [ 'en' ]

os_darwin = false
os_gnu = false
os_freebsd = false
os_windows = false
bin_suffix = ''
lib_suffix = '.so'

if host_machine.system() == 'darwin'
  os_darwin = true
  lib_suffix = '.dylib'
elif host_machine.system() == 'linux'
  os_gnu = true
elif host_machine.system() == 'freebsd'
  os_freebsd = true
elif host_machine.system() == 'windows'
  os_windows = true
  bin_suffix = '.exe'
  lib_suffix = '.dll'
endif

prog_name = get_option ('program_name')
prog_name_lowercase = prog_name.to_lower()
have_custom_name = prog_name != 'Zrythm'
have_custom_logo_and_splash = get_option ('custom_logo_and_splash')

cdata = configuration_data ()
cdata.set_quoted ('PROGRAM_NAME', prog_name)
cdata.set_quoted (
  'PROGRAM_NAME_LOWERCASE', prog_name_lowercase)
if have_custom_name
  cdata.set ('HAVE_CUSTOM_NAME', 1)
endif
if have_custom_logo_and_splash
  cdata.set ('HAVE_CUSTOM_LOGO_AND_SPLASH', 1)
endif
cdata.set_quoted (
  'PACKAGE_VERSION', '@VCS_TAG@')
cdata.set_quoted (
  'COMPILER',
  meson.get_compiler('c').get_id())
cdata.set_quoted ('PREFIX', prefix)
cdata.set_quoted (
  'COMPILER_VERSION',
  meson.get_compiler('c').version())
cdata.set_quoted ('CONFIGURE_DATADIR', datadir)
cdata.set_quoted (
  'CONFIGURE_SOURCEVIEW_STYLES_DIR',
  sourceviewstylesdir)
cdata.set_quoted ('CONFIGURE_THEMES_DIR', themesdir)
cdata.set_quoted (
  'LIBDIR_NAME', get_option ('libdir'))
cdata.set_quoted ('CONFIGURE_LIBDIR', libdir)
cdata.set_quoted ('CONFIGURE_BINDIR', bindir)
cdata.set_quoted ('LIB_SUFFIX', lib_suffix)
cdata.set_quoted ('BIN_SUFFIX', bin_suffix)
cdata.set_quoted ('GSCHEMAS_DIR', schemasdir)
cdata.set_quoted (
  'BUILD_TYPE', get_option ('buildtype'))
if get_option ('trial_ver')
  cdata.set ('TRIAL_VER', 1)
  cdata.set ('TRIAL_LIMIT_MINS', 30)
endif
if get_option ('windows_release')
  cdata.set ('WINDOWS_RELEASE', 1)
endif
if get_option ('mac_release')
  cdata.set ('MAC_RELEASE', 1)
endif
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
zix_inc = include_directories (
  join_paths ('ext', 'zix'))
weakjack_inc = include_directories (
  join_paths ('ext', 'weakjack'))
suil_inc = include_directories (
  join_paths ('inc', 'plugins', 'lv2'))
all_inc = [
  root_inc,
  inc_inc,
  ext_inc,
  midilib_inc,
  zix_inc,
  weakjack_inc,
  suil_inc,
  ]

resources_dir = join_paths (
  meson.source_root (), 'resources')
data_dir = join_paths (
  meson.source_root (), 'data')

cc = meson.get_compiler ('c')

# --- Check for programs ---

find_program ('sed', required: true)
find_program ('gettext', required: true)
dot_bin = find_program ('dot', required: false)

# find plugins for testing
ext_lv2_plugins = {
  'ams_lfo': ['AMS LFO', 'http://github.com/blablack/ams-lv2/lfo'],
  'helm': ['Helm', 'http://tytel.org/helm'],
  'sherlock_atom_inspector': ['Sherlock Atom Inspector', 'http://open-music-kontrollers.ch/lv2/sherlock#atom_inspector'],
  'lsp_compressor': ['LSP Compressor', 'http://lsp-plug.in/plugins/lv2/compressor_stereo'],
  'carla_rack': ['Carla Rack', 'http://kxstudio.sf.net/carla/plugins/carlarack'],
  'no_delay_line': ['No Delay Line', 'http://gareus.org/oss/lv2/nodelay'],
  'mda_ambience': ['mda Ambience', 'http://drobilla.net/plugins/mda/Ambience'],
  }
have_ext_lv2_plugins = {}
ext_lv2_plugin_bundles = {}
if get_option ('tests')
  lv2ls_bin = find_program ('lv2ls', required: false)
  lv2info_bin = find_program (
    'lv2info', required: false)
  get_lv2_bundle_uri_bin = find_program (
    'tools/get_lv2_bundle_uri.sh')
  if lv2ls_bin.found () and lv2info_bin.found ()
    lv2ls_res = run_command (
      lv2ls_bin, check: true).stdout ()
    foreach name, info : ext_lv2_plugins
      uri = info[1]
      have_ext_lv2 = lv2ls_res.contains (uri)
      have_ext_lv2_plugins += {
        name: have_ext_lv2
        }
      if have_ext_lv2
        ext_lv2_plugin_bundles += {
          name: run_command (
            get_lv2_bundle_uri_bin, lv2info_bin,
            uri, check: true).stdout ().strip ()
          }
      endif
    endforeach
  endif
endif

# find command to open a directory
if os_gnu or os_freebsd
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
if not jack_dep.found ()
  jack_dep = dependency (
    'jack',
    required: get_option ('jack'))
endif
have_jack = jack_dep.found () and not get_option ('jack').disabled ()
if have_jack
  cdata.set('HAVE_JACK', 1)
  if jack_dep.version().version_compare('>=1.0')
    cdata.set('HAVE_JACK2', 1)
  endif
  check_functions += [
    ['jack_set_property',[jack_dep]],
    # do not add support for this - it causes
    # problems with jack1/jack2 mixups
    #['jack_client_stop_thread', [jack_dep]],
    ]
endif

glib_dep = dependency('glib-2.0')
check_functions += [
  # use a fallback if this does not exist (glib < 2.58)
  ['g_canonicalize_filename',[glib_dep]],
  ['g_get_console_charset',[glib_dep]],
  ]

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
  # dummy macro for extracting translatable strings
  # when we can't use gettext()
  '-D__(x)=x',
  # use structured log
  '-DG_LOG_USE_STRUCTURED=1',
  ]

if os_windows
  test_cflags += [
    '-DPATH_SPLIT=";"',
    '-DUSE_WEAK_JACK=1',
    ]
else
  test_cflags += [
    '-DPATH_SPLIT=":"',
    ]
endif

if get_option ('profiling')
  test_cflags += [ '-pg', 'no-pie' ]
  if get_option ('extra_optimizations')
    error ('extra_optimizations and profiling are incompatible')
  endif
endif

if get_option ('extra_optimizations')
  if host_machine.cpu() == 'x86_64'
    test_cflags += [
      '-ffast-math',
      # this may break the backtrace
      '-fomit-frame-pointer',
      '-fstrength-reduce',
      '-DPIC',
      '-fdata-sections',
      '-ffunction-sections',
      '-mtune=generic',
      '-msse',
      '-msse2',
      '-mfpmath=sse',
      #'-fvisibility=hidden',
      ]
  endif
endif

if get_option ('extra_debug_info')
  test_cflags += '-g3'
endif

common_cflags = cc.get_supported_arguments (
  test_cflags)

if os_freebsd
  common_cflags += [
    '-I' + includedir,
    ]
elif os_windows
  common_cflags += [
    '-mms-bitfields',
    '-mwindows',
    #'-mstackrealign',
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
  '-Werror=endif-labels',
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

if get_option ('profiling')
  test_ldflags += [ '-pg', 'no-pie' ]
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
# disable for now
if libgtop_dep.found () and false
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
  'libcyaml', required: false, version: '>=1.1.0')
if not cyaml_dep.found()
  cyaml_proj = subproject('libcyaml')
  cyaml_dep = cyaml_proj.get_variable(
    'zrythm_cyaml_dep')
endif
audec_dep = dependency(
  'audec', version: '>=0.2.1',
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

gtksource_dep = dependency (
  'gtksourceview-4', required: false)
if gtksource_dep.found()
  cdata.set('HAVE_GTK_SOURCE_VIEW_4', 1)
else
  gtksource_dep = dependency (
    'gtksourceview-3.0')
  cdata.set('HAVE_GTK_SOURCE_VIEW_3', 1)
endif

libcgraph_dep = dependency (
  'libcgraph',
  required: get_option ('graphviz'))
libgvc_dep = dependency (
  'libgvc',
  required: get_option ('graphviz'))
have_graphviz = libcgraph_dep.found () and libgvc_dep.found ()
if have_graphviz
  cdata.set('HAVE_CGRAPH', 1)
endif

guile_dep = dependency (
  'guile-3.0', required: false)
if not guile_dep.found ()
  guile_dep = dependency (
    'guile-2.2',
    required: get_option ('guile'))
endif
have_guile = guile_dep.found () and not get_option ('guile').disabled ()
if have_guile
  cdata.set('HAVE_GUILE', 1)
endif

libavcodec_dep = dependency (
  'libavcodec',
  required: get_option ('ffmpeg'))
libavformat_dep = dependency (
  'libavformat',
  required: get_option ('ffmpeg'))
libavutil_dep = dependency (
  'libavformat',
  required: get_option ('ffmpeg'))
if libavcodec_dep.found () and libavformat_dep.found () and libavutil_dep.found ()
  cdata.set('HAVE_FFMPEG', 1)
endif

carla_native_plugin_dep = dependency (
  'carla-native-plugin', version: '>=2.1.0',
  required: get_option ('carla'))
carla_standalone_dep = dependency (
  'carla-standalone', version: '>=2.1.0',
  required: get_option ('carla'))
carla_utils_dep = dependency (
  'carla-utils', version: '>=2.1.0',
  required: get_option ('carla'))
if (carla_native_plugin_dep.found () and carla_standalone_dep.found () and carla_utils_dep.found ())
  carla_libdir = carla_native_plugin_dep.get_pkgconfig_variable('libdir')
  install_data (
    find_program (
      carla_libdir / 'carla-discovery-native.exe',
      carla_libdir /
        'carla-discovery-native').path(),
    install_dir: bindir)
  cdata.set('HAVE_CARLA', 1)

  # install discovery & bridge for 32-bit vst's on
  # windows
  if os_windows
    install_data (
      find_program (
        carla_libdir /
          'carla-discovery-win32.exe').path(),
      install_dir: bindir)
    install_data (
      find_program (
        carla_libdir /
          'carla-bridge-win32.exe').path(),
      install_dir: bindir)
  endif

  # install gnu/linux bridges
  foreach bridge_type : [
    'native', 'lv2-gtk2', 'lv2-gtk3', 'lv2-qt4',
    'lv2-qt5' ]
    carla_bridge = find_program (
        carla_libdir / 'carla-bridge-' + bridge_type,
        required: false)
    if carla_bridge.found() and (os_gnu or os_freebsd)
      cdata.set('HAVE_CARLA_BRIDGE_' + bridge_type.underscorify().to_upper(), 1)
      install_data (
        carla_bridge.path(),
        install_dir: bindir)
    endif
  endforeach
endif

portaudio_dep = dependency (
  # disable until it works
  'portaudio-2.0-dont-use-yet',
  required: get_option ('portaudio'))
if portaudio_dep.found ()
  cdata.set('HAVE_PORT_AUDIO', 1)
endif

rtmidi_dep = dependency (
  'rtmidi', required: get_option ('rtmidi'))
if not rtmidi_dep.found () and not get_option ('rtmidi').disabled ()
  rtmidi_proj = subproject('rtmidi')
  rtmidi_dep = rtmidi_proj.get_variable(
    'zrythm_rtmidi_dep')
endif
if rtmidi_dep.found ()
  cdata.set('HAVE_RTMIDI', 1)
endif

rtaudio_dep = dependency (
  'rtaudio', required: get_option ('rtaudio'),
  version: '>=5.1.0')
if not rtaudio_dep.found() and not get_option ('rtaudio').disabled ()
  rtaudio_proj = subproject('rtaudio')
  rtaudio_dep = rtaudio_proj.get_variable(
    'zrythm_rtaudio_dep')
endif
if rtaudio_dep.found ()
  cdata.set('HAVE_RTAUDIO', 1)
endif

sdl_dep = dependency (
  'sdl2', required: get_option ('sdl'))
if sdl_dep.found ()
  cdata.set('HAVE_SDL', 1)
endif

zstd_dep = dependency ('libzstd', required: false)
if not zstd_dep.found ()
  zstd_dep = cc.find_library ('zstd')
endif

# TODO add Cantarell font as dependency

zrythm_deps = [
  gtk_dep,
  sndfile_dep,
  libgtop_dep,
  alsa_dep,
  cyaml_dep,
  audec_dep,
  libcgraph_dep,
  libgvc_dep,
  guile_dep,
  libavcodec_dep,
  libavformat_dep,
  libavutil_dep,
  carla_native_plugin_dep,
  carla_standalone_dep,
  carla_utils_dep,
  portaudio_dep,
  rtaudio_dep,
  rtmidi_dep,
  sdl_dep,
  zstd_dep,
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
  gtksource_dep,

  # these are needed for gentoo
  dependency('fftw3_threads', required: false),
  dependency('fftw3f_threads', required: false),

  libm,
]

if os_windows
  zrythm_deps += cc.find_library('dbghelp')
  # msys2 provides a separate fftw3f entry in
  # pkg-config
  zrythm_deps += dependency('fftw3f')
  zrythm_deps += jack_dep.partial_dependency (
    link_args: false, links: false)
else
  zrythm_deps += jack_dep
endif

# for mingw on arch linux
zrythm_deps += cc.find_library('fftw3_threads', required: false)
zrythm_deps += cc.find_library('fftw3f_threads', required: false)

if os_gnu
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
if get_option ('fallback_version') == ''
  version_fallback = meson.project_version ()
else
  version_fallback = get_option ('fallback_version')
endif
config_h_vcs = vcs_tag (
  input: tmp_h,
  output: 'zrythm-config.h',
  fallback: version_fallback,
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

subdir ('scripts')
subdir ('po')
subdir ('ext')
subdir ('resources')
subdir ('data')
subdir ('src')
subdir ('tests')
subdir ('doc')

meson.add_install_script (
  meson.build_root() / 'scripts' / 'meson_post_install_wrap.sh')

meson.add_dist_script (
  'scripts' / 'meson_dist.sh')

summary ({
  'Program name': get_option('program_name'),
  'Build type': get_option('buildtype'),
  'Profiling': get_option('profiling'),
  'Strict flags': get_option('strict_flags'),
  'Extra optimizations': get_option('extra_optimizations'),
  'Build/install manpage': get_option('manpage'),
  'Build/install user manual': get_option('user_manual'),
  'Install DSEG font': get_option('dseg_font'),
  'Custom logo and splash': get_option('custom_logo_and_splash'),
  }, section: 'General')

summary ({
  'FFmpeg': libavcodec_dep.found () and libavformat_dep.found () and libavutil_dep.found (),
  'Jack': have_jack,
  'PortAudio': portaudio_dep.found (),
  'RtMidi': rtmidi_dep.found (),
  'RtAudio': rtaudio_dep.found (),
  'SDL': sdl_dep.found (),
  'Carla': carla_native_plugin_dep.found () and carla_standalone_dep.found () and carla_utils_dep.found (),
  'Gtop': libgtop_dep.found(),
  'Guile': have_guile,
  'GraphViz': have_graphviz,
  }, section: 'Features')

tests_summary = {
  'Build tests': get_option('tests'),
  'Plugin tests': get_option('plugin_tests'),
  'GUI tests': get_option('gui_tests'),
  'Coverage reports': get_option('b_coverage'),
  }
foreach name, info : ext_lv2_plugins
  tests_summary += {
    info[0]: ext_lv2_plugin_bundles.get(name, 'N/A')
    }
endforeach
summary (tests_summary, section: 'Tests')

summary ({
  'prefix': prefix,
  'includedir': includedir,
  'libdir': libdir,
  'datadir': datadir,
  'sysconfdir': sysconfdir,
  }, section: 'Directories')
