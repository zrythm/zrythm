# Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
  version: '1.0.0-alpha.13.0.4',
  license: 'AGPL-3-or-later',
  meson_version: '>= 0.57.0',
  default_options: [
    'warning_level=2',
    'buildtype=debugoptimized',
    'c_std=gnu11',
    ],
  )

meson_src_root = meson.current_source_dir ()
meson_build_root = meson.current_build_dir ()

prog_name = get_option ('program_name')
prog_name_lowercase = prog_name.to_lower()

gnome = import ('gnome')
fs = import ('fs')
cmake = import ('cmake')
cmake_opts = cmake.subproject_options ()
cmake_opts.add_cmake_defines ({'CMAKE_POSITION_INDEPENDENT_CODE': 'ON'})

prefix = get_option ('prefix')
bindir = prefix / get_option ('bindir')
libdir = prefix / get_option ('libdir')
libexecdir = prefix / get_option ('libexecdir')
includedir = prefix / get_option ('includedir')
datadir = prefix / get_option ('datadir')
sysconfdir = prefix / get_option ('sysconfdir')
mandir = datadir / 'man' / 'man1'
schemasdir = datadir / 'glib-2.0/schemas'
fontsdir = datadir / 'fonts' / prog_name_lowercase
zrythmdatadir = datadir / prog_name_lowercase
themesdir = zrythmdatadir / 'themes'
samplesdir = zrythmdatadir / 'samples'
scriptsdir = zrythmdatadir / 'scripts'
docdir = datadir / 'doc' / prog_name_lowercase
sourceviewstylesdir = zrythmdatadir / 'sourceview-styles'

# Used for building manpages, manual, etc., using
# foreach
language_mappings = {
  'af_ZA': 'Afrikaans',
  'ar': 'اَلْعَرَبِيَّةُ‎',
  'cs': 'Czech',
  'da': 'Dansk',
  'de': 'Deutsch',
  'en': 'English',
  'en_GB': 'English UK',
  'el': 'Ελληνικά',
  'es': 'Español',
  'et': 'Eeti',
  'fa': 'فارسی',
  'fi': 'Suomi',
  'fr': 'Français',
  'gd': 'Gaelic',
  'gl': 'Galego',
  'hi': 'हिन्दी',
  'it': 'Italiano',
  'ja': '日本語',
  'ko': '한국어',
  'nb_NO': 'Bokmål',
  'nl': 'Nederlands',
  'pl': 'Polski',
  'pt': 'Português',
  'pt_BR': 'Português BR',
  'ru': 'русский',
  'sv': 'Svenska',
  'zh_CN': '简体中文',
  'zh_TW': '繁體中文',
  }
locales_str = run_command ('cat', 'po/LINGUAS').stdout ().strip ()
locales = {}
foreach locale : locales_str.split (' ')
  locales += { locale: language_mappings[locale] }
  if locale == 'de'
    locales += { 'en': language_mappings['en'], }
  endif
endforeach
locales_str = ' '.join (locales.keys())

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
else
  error ('unknown host system ' + host_machine.system ())
endif

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
  'HOST_MACHINE_SYSTEM', host_machine.system ())
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
cdata.set_quoted (
  'CONFIGURE_THEMES_DIR', themesdir)
cdata.set_quoted (
  'LIBDIR_NAME', get_option ('libdir'))
cdata.set_quoted ('CONFIGURE_LIBDIR', libdir)
cdata.set_quoted ('CONFIGURE_BINDIR', bindir)
cdata.set_quoted ('LIB_SUFFIX', lib_suffix)
cdata.set_quoted ('BIN_SUFFIX', bin_suffix)
cdata.set_quoted ('GSCHEMAS_DIR', schemasdir)
cdata.set_quoted (
  'BUILD_TYPE', get_option ('buildtype'))
cdata.set_quoted (
  'INSTALLER_VERSION_STR',
  get_option ('buildtype'))
if get_option ('trial_ver')
  cdata.set ('TRIAL_VER', 1)
  cdata.set ('TRIAL_LIMIT_MINS', 60)
endif
if get_option ('installer_ver')
  cdata.set ('INSTALLER_VER', 1)
endif
cdata.set (
  'MESON_SOURCE_ROOT', meson_src_root)
cdata.set (
  'MESON_BUILD_ROOT', meson_build_root)
cdata.set_quoted (
  'ISSUE_TRACKER_URL',
  'https://sr.ht/~alextee/zrythm/trackers')
cdata.set_quoted (
  'NEW_ISSUE_URL',
  'https://todo.sr.ht/~alextee/zrythm-bug')
cdata.set_quoted (
  'NEW_ISSUE_EMAIL',
  '~alextee/zrythm-bug@todo.sr.ht')
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
midilib_inc = include_directories ('ext/midilib')
whereami_inc = include_directories ('ext/whereami')
zix_inc = include_directories ('ext/zix')
weakjack_inc = include_directories ('ext/weakjack')
suil_inc = include_directories ('inc/plugins/lv2')
all_inc = [
  root_inc,
  inc_inc,
  ext_inc,
  midilib_inc,
  whereami_inc,
  zix_inc,
  weakjack_inc,
  suil_inc,
  ]

resources_dir = meson_src_root / 'resources'
data_dir = meson_src_root / 'data'

cc = meson.get_compiler ('c')

# --- Check for programs ---

find_program ('sed')
find_program ('gettext')
msgattrib = find_program ('msgattrib')
msguniq = find_program ('msguniq')
bash = find_program ('bash')
grep = find_program ('grep')
msgfmt = find_program ('msgfmt', required: false)
dot_bin = find_program ('dot', required: false)
rubberband = find_program (
  'rubberband', required: false)

ulimit = find_program (
  'scripts/have_unlimited_mem.sh')
if os_gnu
  have_unlimited_mem = run_command (
    ulimit, check: true).stdout ().strip () == 'unlimited'
  if have_unlimited_mem
    cdata.set ('HAVE_UNLIMITED_MEM', 1)
  endif
endif

# find plugins for testing
ext_lv2_plugins = {
  'ams_lfo': [
    'AMS LFO',
    'http://github.com/blablack/ams-lv2/lfo'],
  'helm': [
    'Helm', 'http://tytel.org/helm'],
  'sherlock_atom_inspector': [
    'Sherlock Atom Inspector',
    'http://open-music-kontrollers.ch/lv2/sherlock#atom_inspector'],
  'lsp_compressor': [
    'LSP Compressor',
    'http://lsp-plug.in/plugins/lv2/compressor_stereo'],
  'lsp_sidechain_compressor': [
    'LSP Sidechain Compressor',
    'http://lsp-plug.in/plugins/lv2/sc_compressor_stereo'],
  'lsp_multisampler_24_do': [
    'LSP MultiSampler x24 Direct Out',
    'http://lsp-plug.in/plugins/lv2/multisampler_x24_do'],
  'carla_rack': [
    'Carla Rack',
    'http://kxstudio.sf.net/carla/plugins/carlarack'],
  'no_delay_line': [
    'No Delay Line',
    'http://gareus.org/oss/lv2/nodelay'],
  'mda_ambience': [
    'mda Ambience',
    'http://drobilla.net/plugins/mda/Ambience'],
  'midi_cc_map': [
    'MIDI CC Map',
    'http://gareus.org/oss/lv2/midifilter#mapcc'],
  'noize_maker': [
    'NoizeMak3r',
    'http://kunz.corrupt.ch/products/tal-noisemaker'],
  'tal_filter': [
    'TAL Filter',
    'urn:juce:TalFilter'],
  'geonkick': [
    'Geonkick',
    'http://geontime.com/geonkick/single'],
  'chipwave': [
    'ChipWave',
    'https://github.com/linuxmao-org/shiru-plugins/chipwave'],
  'calf_compressor': [
    'Calf Compressor',
    'http://calf.sourceforge.net/plugins/Compressor'],
  }
have_ext_lv2_plugins = {}
ext_lv2_plugin_bundles = {}
ext_vst_plugins = {
  'noizemaker': 'TAL-NoiseMaker.so',
  }
have_ext_vst_plugins = {}
ext_vst_plugin_paths = {}
if get_option ('tests')
  lv2ls_bin = find_program ('lv2ls', required: false)
  lv2info_bin = find_program (
    'lv2info', required: false)
  get_lv2_bundle_uri_bin = find_program (
    'tools/get_lv2_bundle_uri.sh')
  get_vst_path_bin = find_program (
    'tools/get_vst_path.sh')
  # get lv2 bundle paths
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
  # get vst plugin paths
  if get_vst_path_bin.found ()
    foreach name, filename : ext_vst_plugins
      have_ext_vst = run_command (
        get_vst_path_bin,
        filename).returncode () == 0
      have_ext_vst_plugins += {
        name: have_ext_vst
        }
      if have_ext_vst
        ext_vst_plugin_paths += {
          name: run_command (
            get_vst_path_bin, filename,
            check: true).stdout ().strip ()
          }
      endif
    endforeach
  endif
endif

# find command to open a directory
if os_gnu or os_freebsd
  open_dir_cmd = find_program ('xdg-open').full_path ()
elif os_darwin
  open_dir_cmd = 'open'
  find_program (open_dir_cmd)
elif os_windows
  open_dir_cmd = 'explorer.exe'
endif
cdata.set_quoted('OPEN_DIR_CMD', open_dir_cmd)

illegal_chars = [
  ' ', # U+202F
  ]
illegal_char_test_args = [
  '-c', '! "$1" -rn "$2" "$3"', '_ignored',
  grep.full_path (),
  ]

sphinx_build = find_program (
  ['sphinx-build', 'sphinx-build-3'],
  required: get_option ('user_manual'))
sphinx_intl = find_program (
  'sphinx-intl',
  required: get_option ('user_manual'))
sass = find_program (
  'sass', required: get_option ('user_manual'))
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

# --- Check for dependencies/libraries ---

all_static = get_option ('static_deps')

# Maths functions might be implemented in libm
libm = cc.find_library (
  'm', required: false, static: all_static)

check_functions = [
  ['mlock', libm],
  ]

# prefer jack1
jack_dep = dependency (
  'jack', required: false, version: '<1.0',
  static: all_static)
if not jack_dep.found ()
  jack_dep = dependency (
    'jack',
    required: get_option ('jack'),
    static: all_static)
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

glib_dep = dependency (
  'glib-2.0', required: false,
  static: all_static)
if glib_dep.found()
  check_functions += [
    # use a fallback if this does not exist (glib < 2.58)
    ['g_canonicalize_filename',[glib_dep]],
    ['g_get_console_charset',[glib_dep]],
    ]
else
  cdata.set ('GLIB_SUBPROJECT', 1)
  glib_proj = subproject ('glib')
  glib_dep = glib_proj.get_variable ('libglib_dep')
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
  '-Walloca',
  '-fno-common',
  # dummy macro for extracting translatable strings
  # when we can't use gettext()
  '-D__(x)=x',
  # use structured log
  '-DG_LOG_USE_STRUCTURED=1',
  '-DG_LOG_DOMAIN="' + prog_name_lowercase + '"',
  ]

# add attribute macros
attributes = [
  'const', 'cold', 'hot', 'pure', 'nonnull',
  'returns_nonnull', 'stack_protect',
  'no_stack_protector', 'deprecated',
  ]
foreach a : attributes
  test_cflags += '-D' + a.to_upper() + '=__attribute__((' + a + '))'
endforeach
test_cflags += '-DDEPRECATED_MSG(x)=__attribute__((deprecated(x)))'

if cc.get_id() == 'clang'
  test_cflags += [
    '-DREALTIME=__attribute__((annotate("realtime")))',
    ]
else
  test_cflags += [
    '-DREALTIME=',
    ]
endif

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
  if get_option ('extra_debug_info') or get_option ('debug') or get_option ('buildtype').contains ('debug')
    test_cflags += [
      '-fno-omit-frame-pointer',
      ]
  else
    # note: this may break the backtrace
    test_cflags += [
      '-fomit-frame-pointer',
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
  # TODO enable following 2 lines to catch VLAs
  #'-Wvla',
  #'-Werror=vla',
  '-fstrict-aliasing',
  '-Wstrict-aliasing=2',
  '-Werror=strict-aliasing',
  #'-Werror=strict-overflow',
  '-Wstrict-overflow=2',
  '-fstrict-overflow',
  '-Werror=duplicated-branches',
  '-Werror=duplicated-cond',
  #'-Werror=null-dereference',
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
  '-Werror=logical-not-parentheses',
  '-Werror=parentheses',
  '-Werror=comment',
  '-Werror=sizeof-pointer-div',
  '-Werror=shift-count-overflow',
  '-Werror=free-nonheap-object',
  '-fanalyzer',
  '-Werror=analyzer-possible-null-dereference',
  '-Werror=analyzer-malloc-leak',
  '-Werror=analyzer-null-dereference',
  # gives false positives
  #'-Werror=analyzer-null-argument',
  '-Werror=analyzer-use-after-free',
  '-Werror=analyzer-possible-null-argument',
  '-Werror=analyzer-double-free',
  '-Werror=analyzer-file-leak',
  '-Werror=nonnull',
  '-Werror=nonnull-compare',
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
if not os_darwin
  x11_dep = dependency (
    'x11', required: false, static: all_static)
  if (x11_dep.found ())
    cdata.set('HAVE_X11', 1)
  endif
endif
alsa_dep = dependency (
  'alsa', required: false, static: all_static)
have_alsa = alsa_dep.found ()
if (have_alsa)
  cdata.set('HAVE_ALSA', 1)
endif
pulseaudio_dep = dependency (
  'libpulse', required: false, static: all_static)
have_pulseaudio = pulseaudio_dep.found ()
if (have_pulseaudio)
  cdata.set('HAVE_PULSEAUDIO', 1)
endif
libsoundio_dep = cc.find_library (
  'soundio', required: false, static: all_static)
have_libsoundio = libsoundio_dep.found ()
if (have_libsoundio)
  cdata.set('HAVE_LIBSOUNDIO', 1)
endif
cyaml_dep = dependency(
  'libcyaml', required: false, version: '>=1.1.0',
  static: all_static)
if not cyaml_dep.found()
  cyaml_proj = subproject('libcyaml')
  cyaml_dep = cyaml_proj.get_variable(
    'zrythm_cyaml_dep')
endif
audec_dep = dependency(
  'audec', version: '>=0.3.2',
  fallback: ['libaudec', 'libaudec_dep'],
  required: true, static: all_static)

gtk_dep = dependency (
  'gtk+-3.0', version: '>=3.22',
  fallback: ['gtk3', 'libgtk_dep'],
  static: all_static)
min_version = '>=1.0.25'
if get_option ('opus')
  cdata.set('HAVE_OPUS', 1)
  min_version = '>=1.0.29'
endif
sndfile_dep = dependency (
  'sndfile', version: min_version, required: false)
if not sndfile_dep.found ()
  sndfile_subproject = cmake.subproject (
    'sndfile', options: cmake_opts)
  sndfile_dep = sndfile_subproject.dependency (
    'sndfile')
endif
samplerate_dep = dependency (
  'samplerate', version: '>=0.1.8', required: false)
if not samplerate_dep.found ()
  samplerate_subproject = cmake.subproject (
    'samplerate', options: cmake_opts)
  samplerate_dep = samplerate_subproject.dependency (
    'samplerate')
endif
lv2_dep = dependency (
  'lv2', version: '>=1.16.0',
  fallback: ['lv2', 'lv2_dep'],
  static: all_static)
if lv2_dep.version().version_compare('>=1.18.0')
  cdata.set('HAVE_LV2_1_18', 1)
endif
serd_dep = dependency (
  'serd-0', version: '>=0.30.0',
  fallback: ['serd', 'serd_dep'],
  static: all_static)
sord_dep = dependency (
  'sord-0', version: '>=0.14.0',
  fallback: ['sord', 'sord_dep'],
  static: all_static)
sratom_dep = dependency (
  'sratom-0', version: '>=0.4.0',
  fallback: ['sratom', 'sratom_dep'],
  static: all_static)
lilv_dep = dependency (
  'lilv-0', version: '>=0.24.6',
  fallback: ['lilv', 'lilv_dep'],
  static: all_static)

# fftw
fftw3_deps = [
  dependency (
    'fftw3', version: '>=3.3.5', static: all_static),
  dependency (
    'fftw3_threads', required: false,
    static: all_static),
  dependency (
    'fftw3f_threads', required: false,
    static: all_static),
  ]
if os_darwin or os_freebsd
  fftw3_deps += [
    cc.find_library (
      'fftw3_threads', required: true,
      static: all_static,
      dirs: [ '/usr/lib', '/usr/local/lib' ]),
    cc.find_library (
      'fftw3f_threads', required: true,
      static: all_static,
      dirs: [ '/usr/lib', '/usr/local/lib' ])
    ]
else
  fftw3_deps += [
    cc.find_library (
      'fftw3_threads', required: false,
      static: all_static),
    cc.find_library (
      'fftw3f_threads', required: false,
      static: all_static)
    ]
endif
if os_windows
  # msys2 provides a separate fftw3f entry in
  # pkg-config
  fftw3_deps += dependency (
    'fftw3f', static: all_static)
endif
fftw3_funcs = [
  'fftw_make_planner_thread_safe',
  'fftwf_make_planner_thread_safe',
  ]
foreach func : fftw3_funcs
  if not cc.has_function (func, dependencies: fftw3_deps)
    warning (
      func + ' missing. ' +
      'If linking fails, on some systems you ' +
      'may have to point zrythm to the ' +
      'library that provides it directly, eg., ' +
      'LDFLAGS="$LDFLAGS ' +
      '/usr/lib/libfftw3_threads.so ' +
      '/usr/lib/libfftw3f_threads.so')
  endif
endforeach

chromaprint_dep = dependency (
  'libchromaprint', required: false,
  static: all_static)
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
  required: get_option ('graphviz'),
  static: all_static)
libgvc_dep = dependency (
  'libgvc',
  required: get_option ('graphviz'),
  static: all_static)
have_graphviz = libcgraph_dep.found () and libgvc_dep.found ()
if have_graphviz
  cdata.set('HAVE_CGRAPH', 1)
endif

guile_dep = dependency (
  'guile-3.0', required: false,
  static: all_static)
if not guile_dep.found ()
  guile_dep = dependency (
    'guile-2.2',
    required: get_option ('guile'),
    static: all_static)
endif
have_guile = guile_dep.found () and not get_option ('guile').disabled ()
if have_guile
  cdata.set('HAVE_GUILE', 1)
endif

carla_host_plugin_dep = dependency (
  'carla-host-plugin', version: '>=2.2.91',
  required: get_option ('carla'),
  static: all_static)
have_carla = carla_host_plugin_dep.found ()
if have_carla
  carla_bins_dir = get_option ('carla_binaries_dir')
  if carla_bins_dir == ''
    carla_bins_dir = carla_host_plugin_dep.get_variable (pkgconfig: 'libdir')
  endif
  install_data (
    find_program (
      carla_bins_dir / 'carla-discovery-native.exe',
      carla_bins_dir /
        'carla-discovery-native').full_path(),
    install_dir: bindir)
  cdata.set('HAVE_CARLA', 1)

  # install discovery & bridge for 32-bit vst's on
  # windows
  if os_windows
    install_data (
      find_program (
        carla_bins_dir /
          'carla-discovery-win32.exe').full_path(),
      install_dir: bindir)
    install_data (
      find_program (
        carla_bins_dir /
          'carla-bridge-win32.exe').full_path(),
      install_dir: bindir)
  endif

  # install gnu/linux bridges
  bridge_types = [
    'native', 'lv2-gtk2', 'lv2-gtk3', 'lv2-qt4',
    'lv2-qt5' ]
  foreach bridge_type : bridge_types
    carla_bridge = find_program (
        carla_bins_dir / 'carla-bridge-' + bridge_type,
        required: false)
    if carla_bridge.found() and (os_gnu or os_freebsd)
      cdata.set('HAVE_CARLA_BRIDGE_' + bridge_type.underscorify().to_upper(), 1)
      install_data (
        carla_bridge.full_path(),
        install_dir: bindir)
    endif
  endforeach
endif

portaudio_dep = dependency (
  # disable until it works
  'portaudio-2.0-dont-use-yet',
  required: get_option ('portaudio'),
  static: all_static)
if portaudio_dep.found ()
  cdata.set('HAVE_PORT_AUDIO', 1)
endif

rtmidi_dep = dependency (
  'rtmidi', required: get_option ('rtmidi'),
  static: all_static)
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
  version: '>=5.1.0',
  static: all_static)
if not rtaudio_dep.found() and not get_option ('rtaudio').disabled ()
  rtaudio_proj = subproject('rtaudio')
  rtaudio_dep = rtaudio_proj.get_variable(
    'zrythm_rtaudio_dep')
endif
if rtaudio_dep.found ()
  cdata.set('HAVE_RTAUDIO', 1)
endif

sdl_dep = dependency (
  'sdl2', required: get_option ('sdl'),
  static: all_static)
if sdl_dep.found ()
  cdata.set('HAVE_SDL', 1)
endif

zstd_dep = dependency (
  'libzstd', required: false,
  static: all_static)
if not zstd_dep.found ()
  zstd_dep = cc.find_library (
    'zstd', static: all_static)
endif

reproc_dep = dependency (
  'reproc', version: '>=14.1.0', required: false,
  static: all_static)
if not reproc_dep.found ()
  reproc_subproject = cmake.subproject (
    'reproc', options: cmake_opts)
  reproc_dep = reproc_subproject.dependency (
    'reproc')
endif

lsp_dsp_dep = dependency (
  'lsp-dsp-lib', version: '>=0.5.5',
  static: all_static or get_option ('static_lsp_dsp'),
  required: get_option ('lsp_dsp'))
have_lsp_dsp = lsp_dsp_dep.found () and not get_option ('lsp_dsp').disabled ()
if have_lsp_dsp
  cdata.set('HAVE_LSP_DSP', 1)
endif

# TODO add Cantarell font as dependency

zrythm_deps = [
  gtk_dep,
  sndfile_dep,
  samplerate_dep,
  alsa_dep,
  pulseaudio_dep,
  libsoundio_dep,
  cyaml_dep,
  audec_dep,
  libcgraph_dep,
  libgvc_dep,
  guile_dep,
  carla_host_plugin_dep,
  portaudio_dep,
  rtaudio_dep,
  rtmidi_dep,
  sdl_dep,
  zstd_dep,
  reproc_dep,
  dependency ('threads'),
  lv2_dep,
  serd_dep,
  sord_dep,
  sratom_dep,
  lilv_dep,
  dependency ('rubberband'),
  dependency ('libpcre'),
  cc.find_library ('dl', static: all_static),
  chromaprint_dep,
  fftw3_deps,
  gtksource_dep,
  lsp_dsp_dep,

  libm,
]

if all_static
  zrythm_deps += [
    # provided by bzip2
    cc.find_library (
      'bz2', static: all_static),
    # provided by lzip
    cc.find_library (
      'lzma', static: all_static),
    ]
endif

if os_windows
  # TODO disable dbghelp and enable drmingw when
  # meson issue is fixed
  zrythm_deps += cc.find_library ('dbghelp')
  #drmingw_subproject = cmake.subproject (
  #  'drmingw', options: cmake_opts)
  #drmingw_dep = drmingw_subproject.dependency (
  #  'drmingw')
  #zrythm_deps += drmingw_dep
  zrythm_deps += jack_dep.partial_dependency (
    link_args: false, links: false)
else
  zrythm_deps += jack_dep
endif

if os_gnu
  zrythm_deps += cc.find_library (
    'rt', static: all_static)
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
  zrythm_deps += x11_dep
endif

# copy doc files to build dir
root_doc_files = [
  'HACKING.md',
  'AUTHORS',
  'TRANSLATORS',
  'THANKS',
  'INSTALL.md',
  ]
foreach f : root_doc_files
  configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach

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
subdir ('inc')
subdir ('src')
subdir ('tests')
subdir ('doc')

meson.add_install_script (
  'scripts/meson-post-install.sh')

meson.add_dist_script (
  'scripts/meson_dist.sh')

summary ({
  'Program name': get_option ('program_name'),
  'Build type': get_option ('buildtype'),
  'Profiling': get_option ('profiling'),
  'Strict flags': get_option ('strict_flags'),
  'Static deps': get_option ('static_deps'),
  'Extra optimizations': get_option ('extra_optimizations'),
  'Build/install manpage': get_option ('manpage'),
  'Build/install user manual': get_option ('user_manual'),
  'Install DSEG font': get_option ('dseg_font'),
  'Custom logo and splash': get_option ('custom_logo_and_splash'),
  'Trial version': get_option ('trial_ver'),
  'Installer version': get_option ('installer_ver'),
  }, section: 'General')

summary ({
  'ALSA': have_alsa,
  'Carla': have_carla,
  'GraphViz': have_graphviz,
  'Guile': have_guile,
  'JACK': have_jack,
  'SoundIO': have_libsoundio,
  'LSP DSP': have_lsp_dsp,
  'PortAudio': portaudio_dep.found (),
  'PulseAudio': have_pulseaudio,
  'RtAudio': rtaudio_dep.found (),
  'RtMidi': rtmidi_dep.found (),
  'SDL': sdl_dep.found (),
  }, section: 'Features')

if have_carla
  # TODO
  summary ({
    }, section: 'Carla bridges')
endif

tests_summary = {
  'Build tests': get_option('tests'),
  'GUI tests': get_option('gui_tests'),
  'Coverage reports': get_option('b_coverage'),
  }
foreach name, info : ext_lv2_plugins
  tests_summary += {
    info[0]: ext_lv2_plugin_bundles.get(name, 'N/A')
    }
endforeach
foreach name, filename : ext_vst_plugins
  tests_summary += {
    filename: ext_vst_plugin_paths.get(name, 'N/A')
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
