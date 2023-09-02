# SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

project (
  'zrythm', ['c', 'cpp'],
  version: files ('VERSION'),
  license: 'AGPL-3-or-later',
  meson_version: '>= 0.57.0',
  default_options: [
    # 1: -Wall
    # 2: -Wall -Wextra
    # 3: -Wall -Wextra -Wpedantic
    'warning_level=2',
    'buildtype=debugoptimized',
    'c_std=gnu17',
    'cpp_std=gnu++11',
    'b_lto=true',
    'b_lto_threads=2',
    'wrap_mode=nofallback',
    ],
  )

# --- Import/configure required modules ---

gnome = import ('gnome')
fs = import ('fs')
cmake = import ('cmake')
cmake_opts = cmake.subproject_options ()
cmake_opts.add_cmake_defines ({
  'CMAKE_POSITION_INDEPENDENT_CODE': 'ON',
  'BUILD_TESTS': 'OFF',
  })

# --- Check for programs ---

find_program ('sed')
find_program ('gettext')
msgattrib = find_program ('msgattrib')
msguniq = find_program ('msguniq')
msggrep = find_program ('msggrep')
msgfmt = find_program ('msgfmt', required: false)
bash = find_program ('bash')
grep = find_program ('grep')
dot_bin = find_program ('dot', required: false)
doxygen = find_program ('doxygen', required: false)
rubberband = find_program (
  'rubberband', required: false)
cppcheck = find_program (
  'cppcheck', required: false)
clang_tidy = find_program (
  'clang-tidy', required: false)
jq = find_program ('jq', required: false)
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
find_program (
  'flex', required: get_option ('completions'))
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
ulimit = find_program (
  'tools/check_have_unlimited_memlock.sh')
get_lv2_bundle_uri_bin = find_program (
  'tools/get_lv2_bundle_uri.sh')
get_vst_path_bin = find_program (
  'tools/get_vst_path.sh')
manpage_completions_run_sh = find_program (
  'ext/sh-manpage-completions/run.sh')
xmllint = find_program ('xmllint')
iwyu_tool = find_program (
  'iwyu_tool', 'iwyu-tool', required: false)
faust2lv2 = find_program (
  'faust2lv2', required: false)
sox = find_program (
  'sox', required: false)

# --- Set common variables ---

cc = meson.get_compiler ('c')
is_gcc = cc.get_id() == 'gcc'
is_clang = cc.get_id() == 'clang'

meson_src_root = meson.current_source_dir ()
meson_build_root = meson.current_build_dir ()

prog_name = get_option ('program_name')
prog_name_lowercase = prog_name.to_lower()
copyright_name = 'The ' + prog_name + ' contributors'
copyright_years = '2018-2023'

prefix = get_option ('prefix')
bindir = prefix / get_option ('bindir')
libdir = prefix / get_option ('libdir')
libexecdir = prefix / get_option ('libexecdir')
includedir = prefix / get_option ('includedir')
datadir = prefix / get_option ('datadir')
sysconfdir = get_option ('sysconfdir')
mandir = datadir / 'man' / 'man1'
schemasdir = datadir / 'glib-2.0/schemas'
fontsdir = datadir / 'fonts' / prog_name_lowercase
zrythm_libdir = prefix / get_option ('libdir') / 'zrythm'
zrythmdatadir = datadir / prog_name_lowercase
themesdir = zrythmdatadir / 'themes'
themes_css_dir = themesdir / 'css'
samplesdir = zrythmdatadir / 'samples'
scriptsdir = zrythmdatadir / 'scripts'
docdir = datadir / 'doc' / prog_name_lowercase
sourceviewstylesdir = zrythmdatadir / 'sourceview-styles'

resources_dir = meson_src_root / 'resources'
data_dir = meson_src_root / 'data'

flatpak_build = get_option ('for_flathub') or get_option ('flatpak')
installer_ver = get_option ('installer_ver')

# Used for building manpages, manual, etc., using
# foreach.
language_mappings = {
  'af_ZA': 'Afrikaans',
  'ar': 'اَلْعَرَبِيَّةُ‎',
  'ca': 'Català',
  #'cs': 'Czech',
  #'da': 'Dansk',
  'de': 'Deutsch',
  'el': 'Ελληνικά',
  'en': 'English',
  'en_GB': 'English UK',
  'es': 'Español',
  #'et': 'Eeti',
  'fa': 'فارسی',
  #'fi': 'Suomi',
  'fr': 'Français',
  #'gd': 'Gaelic',
  'gl': 'Galego',
  'he': 'עִבְרִית',
  'hi': 'हिन्दी',
  'hu': 'magyar nyelv',
  'id': 'bahasa Indonesia',
  'it': 'Italiano',
  'ja': '日本語',
  'ko': '한국어',
  'nb_NO': 'Bokmål',
  'mk': 'македонски',
  'nl': 'Nederlands',
  'pl': 'Polski',
  'pt': 'Português',
  'pt_BR': 'Português BR',
  'ru': 'Русский',
  'sv': 'Svenska',
  'th': 'ภาษาไทย',
  'tr': 'Türkiye',
  'uk': 'Українська',
  'vi': 'Tiếng Việt',
  'zh_CN': '简体中文',
  'zh_TW': '繁體中文',
  }
locales = language_mappings
locales_str = ' '.join (locales.keys())

# get host system and library suffix
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

if get_option ('fallback_version') == ''
  version_fallback = meson.project_version ()
else
  version_fallback = get_option ('fallback_version')
endif

# add global arguments
if os_windows
  # if not building an msys2 package, this might
  # also need -Wl,-allow-multiple-definition
  # see https://stackoverflow.com/questions/11267877/mingw-multiple-definition-of-unwind-resume
  add_global_link_arguments (
    '-lssp',
    language: [ 'c', 'cpp' ])
endif

have_custom_name = prog_name != 'Zrythm'
have_custom_logo_and_splash = get_option ('custom_logo_and_splash')

# include dirs
root_inc = include_directories ('.')
inc_inc = include_directories ('inc')
ext_inc = include_directories ('ext')
kissfft_inc = include_directories ('ext/kissfft')
midilib_inc = include_directories ('ext/midilib')
suil_inc = include_directories ('inc/plugins/lv2')
whereami_inc = include_directories ('ext/whereami')
weakjack_inc = include_directories ('ext/weakjack')
all_inc = [
  root_inc,
  inc_inc,
  ext_inc,
  kissfft_inc,
  midilib_inc,
  suil_inc,
  whereami_inc,
  weakjack_inc,
  ]

all_static = get_option ('static_deps')

# command to open a directory
if os_gnu or os_freebsd
  open_dir_cmd = find_program ('xdg-open').full_path ()
elif os_darwin
  open_dir_cmd = 'open'
  find_program (open_dir_cmd)
elif os_windows
  open_dir_cmd = 'explorer.exe'
endif

# illegal characters to detect in PO files
illegal_chars = [
  ' ', # U+202F
  ]
illegal_char_test_args = [
  '-c', '! "$1" -rn "$2" "$3"', '_ignored',
  grep.full_path (),
  ]

# GTK/GLIB versions
gtk_req = '4.14.0'
glib_req = '2.74.0'

# --- Add extra languages ---

if os_darwin
  add_languages(['objc', 'objcpp'])
endif

# --- Create configuration data ---
#
# Note: more configuration data might be added after
# this section.

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
  'COPYRIGHT_NAME', copyright_name)
cdata.set_quoted (
  'COPYRIGHT_YEARS', copyright_years)
cdata.set_quoted (
  'HOST_MACHINE_SYSTEM', host_machine.system ())
cdata.set_quoted (
  'PACKAGE_VERSION',
  # vcs_tag() broke on windows builds so just use the given
  # version
  os_windows ? version_fallback : '@VCS_TAG@')
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
  'CONFIGURE_THEMES_CSS_DIR', themes_css_dir)
cdata.set_quoted (
  'LIBDIR_NAME', get_option ('libdir'))
if get_option ('check_updates')
  cdata.set ('CHECK_UPDATES', 1)
endif
cdata.set_quoted ('CONFIGURE_LIBDIR', libdir)
cdata.set_quoted (
  'CONFIGURE_ZRYTHM_LIBDIR', zrythm_libdir)
cdata.set_quoted ('CONFIGURE_BINDIR', bindir)
cdata.set_quoted ('LIB_SUFFIX', lib_suffix)
cdata.set_quoted ('BIN_SUFFIX', bin_suffix)
cdata.set_quoted ('GSCHEMAS_DIR', schemasdir)
if flatpak_build
  cdata.set ('FLATPAK_BUILD', 1)
endif
if get_option ('for_flathub')
  cdata.set ('FOR_FLATHUB', 1)
endif
if get_option ('dseg_font')
  cdata.set ('HAVE_BUNDLED_DSEG', 1)
endif
cdata.set_quoted (
  'BUILD_TYPE', get_option ('buildtype'))
cdata.set_quoted (
  'OPTIMIZATION', get_option ('optimization'))
if get_option ('debug')
  cdata.set ('IS_DEBUG_BUILD', 1)
endif
cdata.set_quoted (
  'INSTALLER_VERSION_STR',
  get_option ('buildtype'))
if get_option ('trial_ver')
  cdata.set ('TRIAL_VER', 1)
  cdata.set ('TRIAL_MAX_TRACKS', 25)
endif
if installer_ver
  cdata.set ('INSTALLER_VER', 1)
endif
if get_option ('appimage')
  cdata.set ('APPIMAGE_BUILD', 1)
endif
cdata.set (
  'MESON_SOURCE_ROOT', meson_src_root)
cdata.set (
  'MESON_BUILD_ROOT', meson_build_root)
cdata.set_quoted('OPEN_DIR_CMD', open_dir_cmd)
cdata.set_quoted (
  'ISSUE_TRACKER_URL',
  'https://sr.ht/~alextee/zrythm/trackers')
cdata.set_quoted (
  'NEW_ISSUE_URL',
  'https://todo.sr.ht/~alextee/zrythm-bug')
cdata.set_quoted (
  'NEW_ISSUE_EMAIL',
  '~alextee/zrythm-bug@todo.sr.ht')
cdata.set_quoted (
  'PRIVACY_POLICY_URL',
  'https://www.zrythm.org/en/privacy.html')
cdata.set_quoted (
  'DONATION_URL',
  'https://www.zrythm.org/en/community.html#donate')
cdata.set_quoted (
  'PURCHASE_URL',
  'https://www.zrythm.org/en/download.html')
cdata.set_quoted (
  'USER_MANUAL_URL',
  'https://manual.zrythm.org/en/index.html')
cdata.set_quoted (
  'LICENSE_URL',
  'https://git.sr.ht/~alextee/zrythm/tree/master/item/COPYING')
cdata.set_quoted (
  'BUG_REPORT_API_ENDPOINT',
  'https://accounts.zrythm.org/api/v1/error-reports/new')
if get_option ('user_manual')
  cdata.set_quoted ('MANUAL_PATH', docdir)
endif
if get_option ('carla_use_cv32_patchbay_variant')
  cdata.set ('CARLA_HAVE_CV32_PATCHBAY_VARIANT', 1)
endif

if os_windows
  cdata.set ('USE_MMCSS_THREAD_PRIORITIES', 1)
endif

# if latest version in changelog is the project's
# version, add the changelog to the config
chlog_full = fs.read ('CHANGELOG.md')
chlog_list = chlog_full.split('\n## [')
chlog = chlog_list[1].strip ()
if chlog.contains (meson.project_version () + '] -')
  chlog_date = chlog.split (' - ')[1].split ('\n')[0]
  chlog = chlog.split (chlog_date + '\n')[1]
  cdata.set ('HAVE_CHANGELOG', 1)
  cdata.set_quoted ('CHANGELOG_DATE', chlog_date)
  cdata.set_quoted ('CHANGELOG_TXT', '\\n'.join (chlog.split ('\n')))
  # quick hack - eventually reuse the XML from appdata.xml
  cdata.set_quoted ('CHANGELOG_TXT_FOR_ABOUT_DIALOG', '</p><p>'.join (chlog.split ('\n')))
endif

if os_gnu
  have_unlimited_mem = run_command (
    ulimit, check: true).stdout ().strip () == 'unlimited'
  if have_unlimited_mem
    cdata.set ('HAVE_UNLIMITED_MEM', 1)
  endif
  cdata.set ('HAVE_C11_THREADS', 1)
endif

# --- Check for size of data types ---

# Require at least 32 bits for compatibility
# between XXH32 hash and GHashTable which requires
# unsigned int hashes
if cc.sizeof ('unsigned int') < 4
  error ('minimum required unsigned int size is 4')
endif

# --- Check for headers ---

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

# math functions might be implemented in libm
libm = cc.find_library (
  'm', required: false, static: all_static)

check_functions = [
  ['mlock', libm],
  ]

# prefer jack1
use_weak_jack = os_windows or get_option ('appimage') or installer_ver
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

glib_version_arr = glib_req.split('.')
glib_major_version = glib_version_arr[0]
glib_minor_version = glib_version_arr[1]
gtk_version_arr = gtk_req.split('.')
gtk_major_version = gtk_version_arr[0]
gtk_minor_version = gtk_version_arr[1]
add_project_arguments([
    '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_@0@_@1@'.format(glib_major_version, glib_minor_version),
    '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_@0@_@1@'.format(glib_major_version, glib_minor_version),
    '-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_@0@_@1@'.format(gtk_major_version, gtk_minor_version),
    '-DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_@0@_@1@'.format(gtk_major_version, 14),
  ],
  language: 'c')

foreach func : check_functions
  if cc.has_function(func[0], dependencies: func[1])
    cdata.set('HAVE_' + func[0].underscorify().to_upper(), 1)
  endif
endforeach

# check standard C functions
check_c_functions = [
  'localtime_r',
  ]
foreach func : check_c_functions
  if not cc.has_function (func)
    cdata.set('HAVE_' + func.underscorify().to_upper(), 1)
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
  '-DLV2_UI__Gtk4UI="https://lv2plug.in/ns/extensions/ui#Gtk4UI"',
  # use structured log
  '-DG_LOG_USE_STRUCTURED=1',
  '-DG_LOG_DOMAIN="' + prog_name_lowercase + '"',
  '-DREALTIME=' + (is_clang ? '__attribute__((annotate("realtime")))' : ''),
  #'-DDEPRECATED_MSG(x)=__attribute__((deprecated(x)))',
  '-DOPTIMIZE(x)=' + (is_gcc ? '__attribute__((optimize(#x)))' : ''),
  '-DOPTIMIZE_O0=OPTIMIZE(O0)',
  '-DOPTIMIZE_O1=OPTIMIZE(O1)',
  '-DOPTIMIZE_O2=OPTIMIZE(O2)',
  '-DOPTIMIZE_O3=OPTIMIZE(O3)',
  '-DNOINLINE=__attribute__((noinline))',
  '-DNONNULL_ARGS(...)=__attribute__((nonnull(__VA_ARGS__)))',
  '-DACCESS(...)=' + (is_gcc ? '__attribute__((access(__VA_ARGS__)))' : ''),
  '-DACCESS_READ_ONLY(...)=' + (is_gcc ? 'ACCESS(read_only,__VA_ARGS__)' : ''),
  ]

# not supported on windows
if is_gcc and not os_windows
  test_cflags += [
    '-frecord-gcc-switches',
    ]
endif

# Stricter flags to be used where needed
test_strict_cflags = []

# add attribute macros
attributes = [
  'always_inline', 'cold', 'hot', 'pure', 'nonnull',
  'returns_nonnull', 'stack_protect',
  'no_stack_protector', 'warn_unused_result',
  'const', 'malloc',
  ]
foreach a : attributes
  test_cflags += '-D' + a.to_upper() + '=__attribute__((' + a + '))'
endforeach

if use_weak_jack
  test_cflags += [
    '-DUSE_WEAK_JACK=1',
    ]
endif

if get_option ('profiling')
  test_cflags += [ '-pg', 'no-pie' ]
  if get_option ('extra_optimizations')
    error ('extra_optimizations and profiling are incompatible')
  endif
endif

if get_option ('native_build')
  test_cflags += [
    '-march=native',
    '-mtune=native',
    ]
endif

common_cflags = []

extra_optimizations_cflags = [
  '-ffast-math',
  '-DPIC',
  '-freciprocal-math',
  '-fsingle-precision-constant',
  ]

if host_machine.cpu() == 'x86_64'
  extra_optimizations_cflags += [
    '-mfpmath=sse',
    ]
endif

if get_option ('extra_debug_info') or get_option ('debug') or get_option ('buildtype').contains ('debug')
  extra_optimizations_cflags += [
    # -O1 turns this on, but it may break backtraces so
    # explicitly turn it off on builds supposed to be
    # debuggable
    '-fno-omit-frame-pointer',
    ]
endif
extra_optimizations_cflags = cc.get_supported_arguments (extra_optimizations_cflags)

if get_option ('extra_optimizations')
  test_cflags += extra_optimizations_cflags
endif

# TODO check
# -Ofast -fgraphite-identity -floop-nest-optimize -fdevirtualize-at-ltrans -fipa-pta -fno-semantic-interposition -flto=4 -fuse-linker-plugin -pipe -falign-functions=32 -floop-nest-optimize -ftree-vectorize -floop-parallelize-all -ftree-parallelize-loops=4
extra_extra_optimizations_cflags = [
  '-fallow-store-data-races',
  '-ffinite-math-only',
  '-fno-signed-zeros',
  '-fno-trapping-math',
  '-ffp-contract=fast',
  '-fmodulo-sched',
  ]
extra_extra_optimizations_cflags = cc.get_supported_arguments (extra_extra_optimizations_cflags)

if get_option ('extra_extra_optimizations')
  test_cflags += extra_extra_optimizations_cflags
endif

if get_option ('extra_debug_info')
  test_cflags += [
    '-g3',
    '-rdynamic',
    '-funwind-tables',
    '-fasynchronous-unwind-tables',
    '-fno-toplevel-reorder',
    ]
endif

common_cflags += cc.get_supported_arguments (
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

test_strict_cflags += [
  '-Werror=format-overflow',
  '-Werror=format-truncation',
  #'-Werror=cast-qual',
  '-Werror=clobbered',

  # FIXME enable and fix all reported errors
  #'-Werror=conversion',

  '-Werror=disabled-optimization',
  '-Werror=float-equal',
  '-Werror=logical-op',
  '-Werror=pointer-arith',
  '-Werror=enum-conversion',
  '-Werror=overlength-strings',
  '-Werror=stringop-truncation',
  '-Werror=missing-declarations',
  '-Werror=int-to-pointer-cast',

  # maybe not needed but keep for future reference
  #'-Werror=double-promotion',

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
  '-Werror=null-dereference',
  '-Werror=init-self',
  '-Werror=jump-misses-init',
  '-Werror=missing-prototypes',
  '-Werror=nested-externs',
  '-Werror=write-strings',
  '-Werror=sign-compare',
  '-Werror=discarded-qualifiers',
  '-Werror=float-conversion',
  '-Werror=implicit-function-declaration',
  '-Werror=uninitialized',
  '-Werror=maybe-uninitialized',
  '-Werror=return-type',
  '-Werror=int-conversion',
  '-Werror=incompatible-pointer-types',
  '-Werror=implicit-int',
  '-Werror=multistatement-macros',
  '-Werror=switch',
  '-Werror=overflow',
  # breaks pango subproject build
  #'-Werror=array-bounds',
  '-Werror=enum-compare',
  '-Werror=misleading-indentation',
  '-Werror=int-in-bool-context',
  '-Werror=type-limits',
  # TODO re-enable after porting gtk widgets
  #'-Werror=deprecated-declarations',
  '-Werror=endif-labels',
  '-Werror=logical-not-parentheses',
  '-Werror=parentheses',
  '-Werror=comment',
  '-Werror=sizeof-pointer-div',
  '-Werror=shift-count-overflow',
  '-Werror=free-nonheap-object',
  '-Werror=use-after-free',
  '-fanalyzer',
  '-Werror=analyzer-possible-null-dereference',
  '-Werror=analyzer-malloc-leak',
  '-Werror=analyzer-null-dereference',
  '-Werror=analyzer-null-argument',
  '-Werror=analyzer-use-after-free',
  '-Werror=analyzer-possible-null-argument',
  '-Werror=analyzer-double-free',
  '-Werror=analyzer-file-leak',
  # GCC still makes mistakes
  #'-Werror=analyzer-use-of-uninitialized-value',
  '-Werror=analyzer-out-of-bounds',
  '-Werror=analyzer-deref-before-check',
  '-Werror=analyzer-allocation-size',
  '-Werror=analyzer-exposure-through-uninit-copy',
  #'-Werror=analyzer-imprecise-fp-arithmetic',
  '-Werror=analyzer-infinite-recursion',
  '-Werror=analyzer-jump-through-null',
  '-Werror=analyzer-fd-leak',
  '-Werror=nonnull',
  '-Werror=nonnull-compare',
  '-Werror=pointer-size',
  '-Werror=override-init',
  '-Werror=bool-compare',
  '-Werror=tautological-compare',
  '-Werror=unused-result',
  #'-Wanalyzer-too-complex',
  #'-Werror=analyzer-too-complex',
  '-Werror=inline',
  '-Werror=duplicate-decl-specifier',
  '-Werror=redundant-decls',
  '-Werror=strict-prototypes',
  '-Werror=sizeof-array-argument',
  '-Werror=lto-type-mismatch',
  '-Werror=odr',
  '-Werror=enum-int-mismatch',
  ]

if is_gcc
  test_strict_cflags += [
    '-Weverything',

    '-Wsuggest-attribute=pure',
    '-Wsuggest-attribute=const',
    '-Wsuggest-attribute=noreturn',
    '-Wsuggest-attribute=format',
    #'-Wsuggest-attribute=malloc',
    '-Wsuggest-attribute=cold',

    # only do the following on GCC because clang
    # reports unnecessary errors
    '-Werror=sign-conversion',
    '-Werror=implicit-fallthrough',
    '-Werror=format=2',
    ]
endif

strict_cflags = []
if get_option ('strict_flags')
  strict_cflags = cc.get_supported_arguments (
    test_strict_cflags)
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
x11_dep = dependency (
  'x11', required: get_option ('x11'), static: all_static)
if (x11_dep.found ())
  cdata.set('HAVE_X11', 1)
endif
alsa_dep = dependency (
  'alsa', required: get_option ('alsa'), static: all_static)
if (alsa_dep.found ())
  cdata.set('HAVE_ALSA', 1)
endif
pulseaudio_dep = dependency (
  'libpulse', required: get_option ('pulse'), static: all_static)
if (pulseaudio_dep.found ())
  cdata.set('HAVE_PULSEAUDIO', 1)
endif
libsoundio_dep = cc.find_library (
  'soundio', required: get_option ('soundio'), static: all_static)
if (libsoundio_dep.found ())
  cdata.set('HAVE_LIBSOUNDIO', 1)
endif
cyaml_dep = dependency(
  'libcyaml', version: '>=1.2.0',
  static: all_static,
  fallback: ['libcyaml', 'zrythm_cyaml_dep'],
  default_options: [
    'default_library=static',
    ])
min_version = '>=1.0.25'
if get_option ('opus')
  cdata.set('HAVE_OPUS', 1)
  min_version = '>=1.0.29'
endif
sndfile_dep = dependency (
  'sndfile', version: min_version,
  fallback: ['libsndfile', 'libsndfile_dep'],
  default_options: [
    'programs=false',
    'default_library=static',
    ])
soxr_dep = dependency('soxr', required: false)
if not soxr_dep.found ()
  # only enable openmp on GNU/Linux to avoid build issues
  if os_gnu
    cmake_opts.add_cmake_defines ({
      'WITH_OPENMP': 'ON'
      })
    cmake_opts.append_compile_args('c', '-fopenmp')
    cmake_opts.append_link_args('-fopenmp')
  else
    cmake_opts.add_cmake_defines ({
      'WITH_OPENMP': 'OFF'
      })
  endif
  soxr_subproject = cmake.subproject (
    'soxr', options: cmake_opts)
  soxr_dep = soxr_subproject.dependency (
    'soxr')
endif

# link to fontconfig/pangoft2 directly because
# they are used in utils/pango.c
fontconfig_dep = dependency (
  'fontconfig', version: '>=2.13.0')
pangoft2_dep = dependency (
  'pangoft2', version: '>=1.50',
  fallback: ['pango', 'libpangoft2_dep'],
  default_options: [
    'default_library=static',
    'introspection=disabled',])

# include separately with -isystem to bypass
# https://github.com/ebassi/graphene/issues/249
# NOTE this won't fix the issue when using graphene as a
# subproject
graphene_dep = dependency (
  'graphene-1.0',
  include_type: 'system')

gtk_dep = dependency (
  'gtk4', version: '>=' + '4.13', #gtk_req,
  fallback: ['gtk4', 'libgtk_dep'],
  default_options: [
    'build-demos=false', 'build-examples=false',
    'build-tests=false',
    'build-testsuite=false',
    'media-gstreamer=disabled',
    'x11-backend=' + (os_gnu ? 'true' : 'false'),
    'introspection=disabled',
    ],
  static: all_static)

libadwaita_dep = dependency (
  'libadwaita-1', version: '>=1.2',
  fallback: ['libadwaita', 'libadwaita_dep'],
  default_options: [
    'vapi=false', 'tests=false',
    'examples=false', 'introspection=disabled',
    'default_library=static'])
if libadwaita_dep.type_name () == 'internal'
  # workaround for resources not found in static subproject
  # from meson devs:
  # "Libraries which use GResource need to provide an init
  # function which registers the gresources"
  # libadwaita currently doesn't register its gresources
  libadwaita_dep = libadwaita_dep.as_link_whole ()
endif

libpanel_dep = dependency (
  'libpanel-1', version: '>=1.0.1',
  fallback: ['libpanel', 'libpanel_static_dep'],
  default_options: [
    'vapi=false',
    'install-examples=false',
    'docs=disabled',
    'introspection=disabled',
    'default_library=static'])

dbus_dep = dependency (
  'dbus-1', required: false)
if (dbus_dep.found ())
  cdata.set('HAVE_DBUS', 1)
endif
lv2_dep = dependency (
  'lv2', version: '>=1.16.0',
  fallback: ['lv2', 'lv2_dep'],
  default_options: [
    'tests=disabled',],
  static: all_static)
if lv2_dep.version().version_compare('>=1.18.0')
  cdata.set('HAVE_LV2_1_18', 1)
endif
serd_dep = dependency (
  'serd-0', version: '>=0.30.0',
  fallback: ['serd', 'serd_dep'],
  default_options: [
    'tests=disabled',
    'default_library=static',
    ],
  static: all_static)
sord_dep = dependency (
  'sord-0', version: '>=0.14.0',
  fallback: ['sord', 'sord_dep'],
  default_options: [
    'tests=disabled',
    'default_library=static',
    ],
  static: all_static)
sratom_dep = dependency (
  'sratom-0', version: '>=0.4.0',
  fallback: ['sratom', 'sratom_dep'],
  default_options: [
    'tests=disabled', 'docs=disabled',
    'default_library=static',
    ],
  static: all_static)
lilv_dep = dependency (
  'lilv-0', version: '>=0.24.6',
  fallback: ['lilv', 'lilv_dep'],
  default_options: [
    'tests=disabled', 'docs=disabled', 'tools=disabled',
    'default_library=static',
    ])

json_glib_dep = dependency (
  'json-glib-1.0',
  fallback: ['json-glib', 'json_glib_dep'],
  default_options: [
    'introspection=disabled', 'gtk_doc=disabled',
    'tests=false',
    'default_library=static',
    ])

# fftw - this is a mess because every distro handles fftw3
# in various different ways
fftw3_deps = [
  dependency (
    'fftw3', version: '>=3.3.5', static: all_static),
  ]
if get_option ('fftw3_threads_separate')
  search_paths = []
  if not os_windows
    search_paths = [
      '/usr/lib', '/usr/local/lib', '/opt/homebrew/lib' ]
  endif
  if get_option ('fftw3_threads_separate_type') == 'auto'
    threads_dep = dependency (
      'fftw3_threads', required: false,
      static: all_static)
    threadsf_dep = dependency (
      'fftw3f_threads', required: false,
      static: all_static)
    if threads_dep.found () and threadsf_dep.found ()
      fftw3_deps += [ threads_dep, threadsf_dep ]
    else
      fftw3_deps += [
        cc.find_library (
          'fftw3_threads', required: true,
          static: all_static,
          dirs: search_paths),
        cc.find_library (
          'fftw3f_threads', required: true,
          static: all_static,
          dirs: search_paths),
        ]
    endif
  elif get_option ('fftw3_threads_separate_type') == 'dependency'
    fftw3_deps += [
      dependency (
        'fftw3_threads', required: true,
        static: all_static),
      dependency (
        'fftw3f_threads', required: true,
        static: all_static),
      ]
  elif get_option ('fftw3_threads_separate_type') == 'library'
    fftw3_deps += [
      cc.find_library (
        'fftw3_threads', required: true,
        static: all_static,
        dirs: search_paths),
      cc.find_library (
        'fftw3f_threads', required: true,
        static: all_static,
        dirs: search_paths),
      ]
  else
    error ('fftw3_threads_separate_type must be either of: library, dependency, auto')
  endif
endif
if os_windows or get_option ('fftw3f_separate')
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
    error (
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
  'gtksourceview-5',
  fallback: ['gtksourceview5', 'gtksource_dep'],
  default_options: [
    'vapi=false', 'introspection=disabled',
    'default_library=static',
    ])

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
  'carla-host-plugin', version: '>=2.6.0',
  required: true, #get_option ('carla'),
  static: all_static)
have_carla = carla_host_plugin_dep.found ()
if have_carla
  carla_bins_dir = get_option ('carla_binaries_dir')
  if carla_bins_dir == ''
    carla_bins_dir = carla_host_plugin_dep.get_variable (pkgconfig: 'carla_libdir')
  endif
  install_data (
    find_program (
      carla_bins_dir / 'carla-discovery-native.exe',
      carla_bins_dir /
        'carla-discovery-native').full_path(),
    install_dir: zrythm_libdir / 'carla')
  cdata.set('HAVE_CARLA', 1)
  cdata.set_quoted('CARLA_VERSION_STRING', carla_host_plugin_dep.version ())

  # install discovery & bridge for 32-bit vst's on
  # windows
  if os_windows
    carla_bins_dir_32bit = get_option ('carla_binaries_dir_32bit')
    install_data (
      find_program (
        carla_bins_dir_32bit /
          'carla-discovery-win32.exe').full_path(),
      install_dir: zrythm_libdir / 'carla')
    install_data (
      find_program (
        carla_bins_dir_32bit /
          'carla-bridge-win32.exe').full_path(),
      install_dir: zrythm_libdir / 'carla')
  endif

  # install gnu/linux bridges
  bridge_types = [
    'native', 'lv2-gtk2', 'lv2-gtk3', 'lv2-qt4',
    'lv2-qt5', 'lv2-x11' ]
  foreach bridge_type : bridge_types
    carla_bridge = find_program (
        carla_bins_dir / 'carla-bridge-' + bridge_type,
        required: false)
    if carla_bridge.found() and (os_gnu or os_freebsd)
      cdata.set('HAVE_CARLA_BRIDGE_' + bridge_type.underscorify().to_upper(), 1)
      install_data (
        carla_bridge.full_path(),
        install_dir: zrythm_libdir / 'carla')
    endif
  endforeach

  if cc.has_function('carla_get_native_patchbay_cv8_plugin', dependencies: carla_host_plugin_dep)
    cdata.set ('CARLA_HAVE_CV8_PATCHBAY_VARIANT', 1)
  endif
  if cc.has_function('carla_get_audio_port_hints', dependencies: carla_host_plugin_dep)
    cdata.set ('CARLA_HAVE_AUDIO_PORT_HINTS', 1)
  endif
endif

rtmidi_dep = dependency (
  'rtmidi', version: '>= 5.0.0',
  required: get_option ('rtmidi'),
  fallback: ['rtmidi', 'zrythm_rtmidi_dep'],
  default_options: [
    'default_library=static'])
if rtmidi_dep.found ()
  cdata.set('HAVE_RTMIDI', 1)
endif

rtaudio_dep = dependency (
  'rtaudio', required: get_option ('rtaudio'),
  version: '>=5.1.0',
  fallback: ['rtaudio', 'rtaudio_dep'],
  default_options: [
    # always disable jack - unnecessary
    #'jack=' + (os_windows or installer_ver ? 'disabled' : 'auto'),
    'jack=disabled',
    # TODO use shared in the future so .dll can be
    # replaced on windows
    'default_library=static',
    # distribution of Zrythm with ASIO support is
    # illegal. see the discussion in
    # https://gitlab.com/gnu-clisp/clisp/blob/dd313099db351c90431c1c90332192edce2bb5c9/doc/Why-CLISP-is-under-GPL
    'asio=disabled',])
if rtaudio_dep.found ()
  cdata.set('HAVE_RTAUDIO', 1)
  rtaudio_ver_arr = rtaudio_dep.version().split('.')
  cdata.set('RTAUDIO_MAJOR_VER', rtaudio_ver_arr[0])
  cdata.set('RTAUDIO_MINOR_VER', rtaudio_ver_arr[1])
endif

sdl_dep = dependency (
  'sdl2', required: get_option ('sdl'),
  fallback: ['sdl2', 'sdl2_dep'],
  default_options: [
    'default_library=static',
    ])
if sdl_dep.found ()
  cdata.set('HAVE_SDL', 1)
endif

zstd_dep = dependency (
  'libzstd',
  fallback: ['zstd', 'libzstd_dep'],
  default_options: [
    'default_library=static',
    ])

reproc_dep = dependency (
  # need windows assertion bugfix in 14.2.1
  'reproc', version: '>=14.2.1', required: false,
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
if lsp_dsp_dep.found ()
  cdata.set('HAVE_LSP_DSP', 1)
endif

valgrind_dep = dependency (
  'valgrind', required: get_option ('valgrind'))
have_valgrind = not get_option ('valgrind').disabled () and valgrind_dep.found ()
if have_valgrind
  cdata.set ('HAVE_VALGRIND', 1)
endif

xxhash_dep = dependency (
  'libxxhash',
  fallback: ['xxhash', 'xxhash_dep'],
  default_options: [
    'default_library=static',
    ])

vamp_static = get_option ('vamp_static')
vamp_dep = dependency ('vamp', static: vamp_static)
vamp_host_sdk_dep = dependency (
  'vamp-hostsdk', static: vamp_static)
vamp_deps = [ vamp_dep, vamp_host_sdk_dep ]

pcre_dep = dependency (
  'libpcre',
  fallback: ['pcre', 'pcre_dep'],
  default_options: [
    'default_library=static',
    ])
pcre2_dep = dependency (
  'libpcre2-8',
  fallback: ['pcre2', 'libpcre2_8'],
  default_options: [
    'default_library=static',
    ])

zix_dep = dependency (
  'zix-0',
  fallback: ['zix', 'zix_dep'],
  default_options: [
    'tests=disabled', 'default_library=static',
    'docs=disabled', 'tests_cpp=disabled',
    ])

# TODO add Cantarell font as dependency

zrythm_deps = [
  libadwaita_dep,
  libpanel_dep,
  fontconfig_dep,
  pangoft2_dep,
  graphene_dep,
  gtk_dep,
  sndfile_dep,
  alsa_dep,
  pulseaudio_dep,
  libsoundio_dep,
  cyaml_dep,
  soxr_dep,
  libcgraph_dep,
  libgvc_dep,
  have_guile ? guile_dep : [],
  carla_host_plugin_dep,
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
  pcre_dep,
  pcre2_dep,
  dependency ('rubberband'),
  dependency ('epoxy'),
  cc.find_library ('dl', static: all_static),
  chromaprint_dep,
  fftw3_deps,
  gtksource_dep,
  lsp_dsp_dep,
  valgrind_dep,
  xxhash_dep,
  vamp_deps,
  dependency ('libcurl'),
  json_glib_dep,
  zix_dep,
  dbus_dep,

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
  # Windows MME
  zrythm_deps += cc.find_library (
    'winmm', static: true)

  # use MgwHelp if found, otherwise regular DbgHelp
  dbg_help_lib = cc.find_library ('mgwhelp', required: false)
  if not dbg_help_lib.found ()
    cc.find_library ('dbghelp')
  endif
  zrythm_deps += dbg_help_lib
else # else if not windows
  libbacktrace_dep = cc.find_library (
    'backtrace', required: false)
  if not libbacktrace_dep.found ()
    libbacktrace_proj = subproject ('libbacktrace')
    libbacktrace_dep = libbacktrace_proj.get_variable ('libbacktrace_dep')
  endif
  zrythm_deps += libbacktrace_dep
  cdata.set ('HAVE_LIBBACKTRACE', 1)
endif

if use_weak_jack
  zrythm_deps += jack_dep.partial_dependency (
    link_args: false, links: false)
else
  zrythm_deps += have_jack ? jack_dep : []
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

# plugins for testing
ext_lv2_plugins = {
  'ams_lfo': [
    'AMS LFO',
    'http://github.com/blablack/ams-lv2/lfo'],
  'calf_monosynth': [
    'Calf Monosynth',
    'http://calf.sourceforge.net/plugins/Monosynth'],
  'helm': [
    'Helm', 'http://tytel.org/helm'],
  'sherlock_atom_inspector': [
    'Sherlock Atom Inspector',
    'http://open-music-kontrollers.ch/lv2/sherlock#atom_inspector'],
  'lsp_compressor_mono': [
    'LSP Compressor Mono',
    'http://lsp-plug.in/plugins/lv2/compressor_mono'],
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
  'mverb': [
    'MVerb',
    'http://distrho.sf.net/plugins/MVerb'],
  'sfizz': [
    'Sfizz',
    'http://sfztools.github.io/sfizz'],
  'drops': [
    'Drops',
    'http://github.com/clearly-broken-software/drops'],
  'test_signal': [
    'Test Signal',
    'http://gareus.org/oss/lv2/testsignal'],
  'kxstudio_lfo': [
    'KXStudio LFO',
    'http://kxstudio.sf.net/carla/plugins/lfo'],
  }
have_ext_lv2_plugins = {}
ext_lv2_plugin_bundles = {}
ext_vst_plugins = {
  'noizemaker': 'TAL-NoiseMaker.so',
  }
have_ext_vst_plugins = {}
ext_vst_plugin_paths = {}
ext_vst3_plugins = {
  'onetrick_simian': 'Punk Labs LLC OneTrick SIMIAN.vst3',
  'dexed': 'Dexed.vst3',
  'surge_xt': 'Surge XT.vst3',
  }
have_ext_vst3_plugins = {}
ext_vst3_plugin_paths = {}
# when using the lilv subproject, we can't use lv2ls and
# lv2info
lv2info_bin = find_program ('lv2info', required: false)
if get_option ('tests') and lilv_dep.type_name () != 'internal'
  lv2ls_bin = find_program ('lv2ls', required: false)
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
            get_vst_path_bin, filename, '',
            check: true).stdout ().strip ()
          }
      endif
    endforeach
    foreach name, filename : ext_vst3_plugins
      have_ext_vst3 = run_command (
        get_vst_path_bin,
        filename).returncode () == 0
      have_ext_vst3_plugins += {
        name: have_ext_vst3
        }
      if have_ext_vst3
        ext_vst3_plugin_paths += {
          name: run_command (
            get_vst_path_bin, filename, '3',
            check: true).stdout ().strip ()
          }
      endif
    endforeach
  endif
endif

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
subdir ('tools')

meson.add_install_script (
  'scripts/meson-post-install.sh')

meson.add_dist_script (
  'scripts/meson_dist.sh')

# --- Run targets ---

if iwyu_tool.found ()
  run_target (
    'iwyu',
    command: [
      iwyu_tool, '-p', meson.project_build_root (),
      '../src',
      ])
endif

# --- Summary ---

summary ({
  'Program name': get_option ('program_name'),
  'Debug': get_option ('debug'),
  'Optimization': get_option ('optimization'),
  'Link-Time Optimization': get_option ('b_lto'),
  'Profiling': get_option ('profiling'),
  'Strict flags': get_option ('strict_flags'),
  'Static deps': get_option ('static_deps'),
  'Extra optimizations': get_option ('extra_optimizations'),
  'Extra extra optimizations': get_option ('extra_extra_optimizations'),
  'Native build': get_option ('native_build'),
  'Bundled plugins': get_option ('bundled_plugins'),
  'Build/install manpage': get_option ('manpage'),
  'Build/install completions': get_option ('completions'),
  'Build/install user manual': get_option ('user_manual'),
  'Install DSEG font': get_option ('dseg_font'),
  'Custom logo and splash': get_option ('custom_logo_and_splash'),
  'Trial version': get_option ('trial_ver'),
  'Installer version': get_option ('installer_ver'),
  'Check for updates': get_option ('check_updates'),
  'AppImage build': get_option ('appimage'),
  }, section: 'General')

summary ({
  'ALSA': alsa_dep.found (),
  'Carla': have_carla,
  'GraphViz': have_graphviz,
  'Guile': have_guile,
  'JACK': have_jack,
  'SoundIO': libsoundio_dep.found (),
  'LSP DSP': lsp_dsp_dep.found (),
  'PulseAudio': pulseaudio_dep.found (),
  'RtAudio': rtaudio_dep.found (),
  'RtMidi': rtmidi_dep.found (),
  'SDL': sdl_dep.found (),
  'X11': x11_dep.found (),
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
  'Valgrind': have_valgrind,
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
foreach name, filename : ext_vst3_plugins
  tests_summary += {
    filename: ext_vst3_plugin_paths.get(name, 'N/A')
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
