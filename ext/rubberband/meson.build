
project(
  'Rubber Band Library',
  'c', 'cpp',
  version: '4.0.0',
  license: 'GPL-2.0-or-later',
  default_options: [
    'cpp_std=c++11',
    'warning_level=3',
    'buildtype=release',
    'default_library=both',
    'b_ndebug=if-release',
    'b_lundef=true',
  ],
  meson_version: '>= 0.53.0'
)

rubberband_dynamic_library_version = '3.0.0'

system = host_machine.system()
architecture = host_machine.cpu_family()
cpp = meson.get_compiler('cpp')

pkg = import('pkgconfig')


# Define the project source sets

public_headers = [
  'rubberband/rubberband-c.h',
  'rubberband/RubberBandStretcher.h',
  'rubberband/RubberBandLiveShifter.h',
]

library_sources = [
  'src/rubberband-c.cpp',
  'src/RubberBandStretcher.cpp',
  'src/RubberBandLiveShifter.cpp',
  'src/faster/AudioCurveCalculator.cpp',
  'src/faster/CompoundAudioCurve.cpp',
  'src/faster/HighFrequencyAudioCurve.cpp',
  'src/faster/SilentAudioCurve.cpp',
  'src/faster/PercussiveAudioCurve.cpp',
  'src/faster/R2Stretcher.cpp',
  'src/faster/StretcherChannelData.cpp',
  'src/faster/StretcherProcess.cpp',
  'src/common/Allocators.cpp',
  'src/common/FFT.cpp',
  'src/common/Log.cpp',
  'src/common/Profiler.cpp',
  'src/common/Resampler.cpp',
  'src/common/StretchCalculator.cpp',
  'src/common/sysutils.cpp',
  'src/common/mathmisc.cpp',
  'src/common/Thread.cpp',
  'src/finer/R3Stretcher.cpp', 
  'src/finer/R3LiveShifter.cpp', 
]

jni_sources = [
  'src/jni/RubberBandStretcherJNI.cpp',
]

java_sources = [
  'com/breakfastquay/rubberband/RubberBandStretcher.java',
  'com/breakfastquay/rubberband/RubberBandLiveShifter.java',
]

java_test_sources = [
  'com/breakfastquay/rubberband/test/RubberBandTest.java',
]

program_sources = [
  'main/main.cpp',
]

if system == 'windows'
  program_sources += [
    'src/ext/getopt/getopt.c',
    'src/ext/getopt/getopt_long.c'
  ]
endif

vamp_sources = [
  'vamp/RubberBandVampPlugin.cpp',
  'vamp/libmain.cpp',
]

ladspa_sources = [
  'ladspa-lv2/libmain-ladspa.cpp',
]

lv2_sources = [
  'ladspa-lv2/libmain-lv2.cpp',
]

unit_test_sources = [
  'src/test/TestAllocators.cpp',
  'src/test/TestFFT.cpp',
  'src/test/TestLiveShifter.cpp',
  'src/test/TestResampler.cpp',
  'src/test/TestVectorOpsComplex.cpp',
  'src/test/TestVectorOps.cpp',
  'src/test/TestSignalBits.cpp',
  'src/test/TestStretchCalculator.cpp',
  'src/test/TestStretcher.cpp',
  'src/test/TestBinClassifier.cpp',
  'src/test/test.cpp',
]

general_include_dirs = [
  'rubberband',
  'src',
]


# Scan for any dependencies we may use later; all are optional

extra_include_args = []
foreach d: get_option('extra_include_dirs')
  extra_include_args += [ '-I' + d ]
endforeach

fftw3_dep = dependency('fftw3', version: '>= 3.0.0', required: false)
sleef_dep = dependency('sleef', version: '>= 3.3.0', required: false)
sleefdft_dep = dependency('sleefdft', version: '>= 3.3.0', required: false)
samplerate_dep = dependency('samplerate', version: '>= 0.1.8', required: false)
speexdsp_dep = dependency('speexdsp', version: '>= 1.0.0', required: false)

sndfile_dep = dependency('sndfile', version: '>= 1.0.16', required: get_option('cmdline'))
vamp_dep = dependency('vamp-sdk', version: '>= 2.9', required: get_option('vamp'))
boost_unit_test_dep = dependency('boost', modules: ['unit_test_framework'], version: '>= 1.73', required: get_option('tests'))
thread_dep = dependency('threads')

have_ladspa = cpp.has_header('ladspa.h', args: extra_include_args, required: get_option('ladspa'))
have_lv2 = cpp.has_header('lv2.h', args: extra_include_args, required: get_option('lv2'))

have_sincos = cpp.has_function('sincos',
                               prefix: '#define _GNU_SOURCE\n#include <math.h>',
                               args: '-lm')


# Do we need libatomic?
#
libatomic_dep = dependency('', required : false)
libatomic_test_program = '''
#include <atomic>
int main() {
    std::atomic<int> i;
    std::atomic<double> d;
    return int(i.is_lock_free() && d.is_lock_free());
}
'''
if cpp.compiles(libatomic_test_program, name : 'test program using std::atomic')
  if (not cpp.links(libatomic_test_program, name : 'test program using std::atomic requires -latomic'))
    libatomic_dep = cpp.find_library('atomic', required : true)
  endif
endif


# Find out whether we can build the optional JNI target. Thanks to Eli
# Schwartz for the fix here; this has been through a few previous
# iterations. Initially we just looked for jni.h, but it's often in an
# obscure place so that didn't work very well. Then tried Meson's
# automatic dependency('jni') handling, but that bails out in Meson
# 0.62/0.63 if javac is not present (even if required is false). Then
# tried guarding that check behind a successful find_program('javac'),
# but that bails out on systems in which a stub javac is installed
# that is not a compiler (because the subsequent Meson jni magic still
# requires javac as a compiler). Now try adding Java as a complete
# language pack, optionally, and only go on to JNI if that succeeds,
# making sure that nothing "clever" happens if Java is not found.
# Note even if jni_dep is found, we still need to check that the
# compiler can find jni.h because the jni autodetection can't really
# be trusted, especially in a macOS frameworks environment.
#
have_jni = false
have_java = add_languages('java', required: get_option('jni'))
if have_java
  # At the time of writing this causes a spurious Meson warning:
  # "Project targets '>= 0.53.0' but uses feature introduced in
  # '0.62.0': dep 'jni' custom lookup." It's spurious because this
  # syntax worked before 0.62 as well, it just didn't invoke the
  # custom lookup
  jni_dep = dependency('jni', required: false)
  have_jni = cpp.has_header('jni.h', dependencies: jni_dep, args: extra_include_args)
endif


# Check FFT and resampler options and set up dependencies and paths

feature_dependencies = []
feature_defines = []
feature_libraries = []
feature_sources = []
pkgconfig_requirements = []
pkgconfig_libraries = []
arch_flags = []

config_summary = {}
target_summary = {}

resampler = get_option('resampler')
fft = get_option('fft')
ipp_path = get_option('ipp_path')
ipp_needed = false

if fft == 'auto'
  if system == 'darwin'
    fft = 'vdsp'
  else
    fft = 'builtin'
  endif
endif

if resampler == 'auto'
  resampler = 'builtin'
endif

if fft == 'builtin'
  config_summary += { 'FFT': 'Built-in' }
  message('For FFT: using built-in implementation')
  if fftw3_dep.found()
    message('(to use FFTW instead, reconfigure with -Dfft=fftw)')
  endif
  if sleef_dep.found()
    message('(to use SLEEF instead, reconfigure with -Dfft=sleef)')
  endif
  feature_defines += ['-DUSE_BUILTIN_FFT']

elif fft == 'kissfft'
  config_summary += { 'FFT': 'KissFFT' }
  message('For FFT: using KissFFT')
  if fftw3_dep.found()
    message('(to use FFTW instead, reconfigure with -Dfft=fftw)')
  endif
  if sleef_dep.found()
    message('(to use SLEEF instead, reconfigure with -Dfft=sleef)')
  endif
  feature_sources += ['src/ext/kissfft/kiss_fft.c', 'src/ext/kissfft/kiss_fftr.c']
  feature_defines += ['-DHAVE_KISSFFT']
  general_include_dirs += 'src/ext/kissfft'

elif fft == 'fftw'
  if not fftw3_dep.found()
    fftw3_dep = cpp.find_library('fftw3',
                                 dirs: get_option('extra_lib_dirs'),
                                 has_headers: ['fftw3.h'],
                                 header_args: extra_include_args,
                                 required: true)
  endif
  config_summary += { 'FFT': 'FFTW' }
  message('For FFT: using FFTW')
  if sleef_dep.found()
    message('(to use SLEEF instead, reconfigure with -Dfft=sleef)')
  endif
  pkgconfig_requirements += fftw3_dep
  feature_dependencies += fftw3_dep
  feature_defines += ['-DHAVE_FFTW3', '-DFFTW_DOUBLE_ONLY']

elif fft == 'sleef'
  if sleefdft_dep.found() and sleef_dep.found()
    config_summary += { 'FFT': 'SLEEF' }
    message('For FFT: using SLEEF')
    pkgconfig_requirements += sleefdft_dep
    pkgconfig_requirements += sleef_dep
  else 
    sleefdft_dep = cpp.find_library('sleefdft',
                                    dirs: get_option('extra_lib_dirs'),
                                    has_headers: ['sleefdft.h'],
                                    header_args: extra_include_args,
                                    required: true)
    sleef_dep = cpp.find_library('sleef',
                                 dirs: get_option('extra_lib_dirs'),
                                 has_headers: ['sleef.h'],
                                 header_args: extra_include_args,
                                 required: true)
    config_summary += { 'FFT': 'SLEEF' }
  endif
  feature_dependencies += sleefdft_dep
  feature_dependencies += sleef_dep
  feature_defines += ['-DHAVE_SLEEF']

elif fft == 'vdsp'
  config_summary += { 'FFT': 'vDSP' }
  message('For FFT: using vDSP')
  feature_defines += ['-DHAVE_VDSP']
  feature_libraries += ['-framework', 'Accelerate']
  pkgconfig_libraries += ['-framework', 'Accelerate']
  
elif fft == 'ipp'
  if ipp_path != ''
    config_summary += { 'FFT': 'Intel IPP' }
    message('For FFT: using IPP')
    message('IPP path defined as ' + ipp_path)
  else 
    error('For FFT: IPP selected, but ipp_path not specified')
  endif
  ipp_needed = true

else
  error('Unknown or unsupported FFT option: ' + fft)

endif # fft

if resampler == 'builtin'
  config_summary += { 'Resampler': 'Built-in' }
  message('For resampler: using built-in implementation')
  if samplerate_dep.found()
    message('(to use libsamplerate instead, reconfigure with -Dresampler=libsamplerate)')
  endif
  library_sources += 'src/common/BQResampler.cpp'
  feature_defines += ['-DUSE_BQRESAMPLER']

elif resampler == 'libsamplerate'
  if not samplerate_dep.found()
    samplerate_dep = cpp.find_library('samplerate',
                                      dirs: get_option('extra_lib_dirs'),
                                      has_headers: ['samplerate.h'],
                                      header_args: extra_include_args,
                                      required: true)
  endif
  config_summary += { 'Resampler': 'libsamplerate' }
  message('For resampler: using libsamplerate')
  feature_dependencies += samplerate_dep
  pkgconfig_requirements += samplerate_dep
  feature_defines += ['-DHAVE_LIBSAMPLERATE']
  
elif resampler == 'speex'
  config_summary += { 'Resampler': 'Speex' }
  message('For resampler: using bundled Speex')
  message('(consider libsamplerate if time-varying pitch shift is required)')
  feature_sources += ['src/ext/speex/resample.c']
  feature_defines += ['-DUSE_SPEEX']
  
elif resampler == 'libspeexdsp'
  if not speexdsp_dep.found()
    speexdsp_dep = cpp.find_library('speexdsp',
                                    dirs: get_option('extra_lib_dirs'),
                                    has_headers: ['speex/speex_resampler.h'],
                                    header_args: extra_include_args,
                                    required: true)
  endif
  config_summary += { 'Resampler': 'Speex DSP' }
  message('For resampler: using Speex DSP library')
  message('(consider libsamplerate if time-varying pitch shift is required)')
  feature_dependencies += speexdsp_dep
  pkgconfig_requirements += speexdsp_dep
  feature_defines += ['-DHAVE_LIBSPEEXDSP']
  
elif resampler == 'ipp'
  if ipp_path != ''
    config_summary += { 'Resampler': 'Intel IPP' }
    message('For resampler: using IPP')
    message('(consider libsamplerate if time-varying pitch shift is required)')
    message('IPP path defined as ' + ipp_path)
  else 
    error('For resampler: IPP selected, but ipp_path not specified')
  endif
  ipp_needed = true

else
  error('Unknown or unsupported resampler option: ' + resampler)

endif # resampler

if not have_sincos
  feature_defines += [ '-DLACK_SINCOS' ]
endif

if ipp_needed
  feature_defines += [
    '-DHAVE_IPP',
    '-DUSE_IPP_STATIC',
    '-I' + ipp_path / 'include'
  ]
  if architecture == 'x86'
    feature_libraries += [
      '-L' + ipp_path / 'lib/ia32',
    ]
  elif architecture == 'x86_64'
    feature_libraries += [
      '-L' + ipp_path / 'lib/intel64',
    ]
  else
    error('IPP is not supported for this architecture')
  endif
  if system == 'windows'
    feature_libraries += [
      '-lippsmt', '-lippvmmt', '-lippcoremt',
    ]
  elif system == 'linux'
    feature_libraries += [
      '-Wl,-Bstatic', '-lipps', '-lippvm', '-lippcore', '-Wl,-Bdynamic',
    ]
  else   
    feature_libraries += [
      '-lipps', '-lippvm', '-lippcore',
    ]
  endif
endif # ipp_needed

if not vamp_dep.found()
  vamp_dep = cpp.find_library('VampPluginSDK',
                              dirs: get_option('extra_lib_dirs'),
                              has_headers: ['vamp-sdk.h'],
                              header_args: extra_include_args,
                              required: get_option('vamp'))
  if not vamp_dep.found()
    vamp_dep = cpp.find_library('vamp-sdk',
                                dirs: get_option('extra_lib_dirs'),
                                has_headers: ['vamp-sdk.h'],
                                header_args: extra_include_args,
                                required: get_option('vamp'))
  endif
endif
have_vamp = vamp_dep.found()

if not sndfile_dep.found()
  sndfile_dep = cpp.find_library('sndfile',
                                 dirs: get_option('extra_lib_dirs'),
                                 has_headers: ['sndfile.h'],
                                 header_args: extra_include_args,
                                 required: get_option('cmdline'))
  if not sndfile_dep.found()
    sndfile_dep = cpp.find_library('sndfile-1',
                                   dirs: get_option('extra_lib_dirs'),
                                   has_headers: ['sndfile.h'],
                                   header_args: extra_include_args,
                                   required: get_option('cmdline'))
  endif
endif
have_sndfile = sndfile_dep.found()

have_boost_unit_test = boost_unit_test_dep.found()


# General platform and compiler expectations

ladspa_symbol_args = []
lv2_symbol_args = []
vamp_symbol_args = []

if get_option('debug')
  config_summary += { 'Debug': 'Enabled' }
else
  config_summary += { 'Debug': 'Disabled' }
  feature_defines += ['-DNO_THREAD_CHECKS', '-DNO_TIMING', '-DNDEBUG']
endif

if system == 'darwin'
  feature_defines += ['-DUSE_PTHREADS', '-DMALLOC_IS_ALIGNED']
  ladspa_symbol_args += [
    '-exported_symbols_list', meson.current_source_dir() / 'ladspa-lv2/ladspa-plugin.list'
  ]
  lv2_symbol_args += [
    '-exported_symbols_list', meson.current_source_dir() / 'ladspa-lv2/lv2-plugin.list'
  ]
  vamp_symbol_args += [
    '-exported_symbols_list', meson.current_source_dir() / 'vamp/vamp-plugin.list'
  ]

  if architecture == 'aarch64'
    arch_flags += [
      '-arch', 'arm64',
    ]
  elif architecture == 'x86_64'
    arch_flags += [
      '-arch', 'x86_64',
    ]
  else # begin architecture != 'aarch64' or 'x86_64'
    error('Build for architecture ' + architecture + ' is not supported on this platform')
  endif # end architecture

  have_version_min = false
  foreach arg: get_option('cpp_args')
    if arg.contains('version-min')
      have_version_min = true
      bits = arg.split('=')
      if bits.length() > 1
	config_summary += { 'Target OS': bits[1] + '+' }
      else
	config_summary += { 'Target OS': '(unknown)' }
      endif
    endif
  endforeach

  if not have_version_min
    message('Using default minimum target OS version')
    message('(consider specifying this in cross-file if earlier target is desired)')
    if architecture == 'aarch64'
      arch_flags += [ '-mmacosx-version-min=11' ]
      config_summary += { 'Target OS': '11+' }
    else 
      arch_flags += [ '-mmacosx-version-min=10.13' ]
      config_summary += { 'Target OS': '10.13+' }
    endif
  endif

elif system == 'windows'
  feature_defines += ['-D_WIN32', '-DNOMINMAX', '-D_USE_MATH_DEFINES', '-DGETOPT_API=']
  if cpp.get_id() == 'msvc'
    ladspa_symbol_args += ['-EXPORT:ladspa_descriptor']
    lv2_symbol_args += ['-EXPORT:lv2_descriptor']
    vamp_symbol_args += ['-EXPORT:vampGetPluginDescriptor']
  endif

else # system not darwin or windows
  feature_defines += ['-DUSE_PTHREADS', '-DHAVE_POSIX_MEMALIGN']
  ladspa_symbol_args += [
    '-Wl,--version-script=' + meson.current_source_dir() / 'ladspa-lv2/ladspa-plugin.map'
  ]
  lv2_symbol_args += [
    '-Wl,--version-script=' + meson.current_source_dir() / 'ladspa-lv2/lv2-plugin.map'
  ]
  vamp_symbol_args += [
    '-Wl,--version-script=' + meson.current_source_dir() / 'vamp/vamp-plugin.map'
  ]
endif # system


general_include_dirs += get_option('extra_include_dirs')
general_compile_args = [ arch_flags, feature_defines ]
general_dependencies = [ feature_dependencies, thread_dep, libatomic_dep ]

rubberband_additional_static_lib = ''

if cpp.get_id() == 'msvc'
  #
  # In the MSVC world we have a quandary, partly as a result of
  # wanting to use naming compatible with our previous/other build
  # systems.
  #
  # Meson would like to use rubberband.dll for the dynamic library,
  # rubberband.lib for the import library, and librubberband.a for the
  # static library. This is kind of ok, even though lib*.a is not a
  # very familiar naming style here - except that previously we called
  # the static library rubberband-static.lib. (It would be usual to
  # expect some .lib file to be produced as a static library,
  # especially if default_library=static is set.)
  #
  # Our "solution" is to leave alone if default_library=shared (when
  # the Meson and MSVC ways are the same), but emit an additional
  # static .lib called rubberband-static.lib otherwise.
  #
  if get_option('default_library') != 'shared'
    rubberband_additional_static_lib = 'rubberband-static'
  endif
  rubberband_library_name = 'rubberband'
  rubberband_program_name = 'rubberband-program'
  rubberband_program_name_r3 = 'rubberband-program-r3'
  rubberband_ladspa_name = 'ladspa-rubberband'
  rubberband_lv2_name = 'lv2-rubberband'
  rubberband_vamp_name = 'vamp-rubberband'
  rubberband_jni_name = 'rubberband-jni'
  unit_tests_name = 'tests'
else
  rubberband_library_name = 'rubberband'
  rubberband_dynamic_name = 'rubberband'
  rubberband_program_name = 'rubberband'
  rubberband_program_name_r3 = 'rubberband-r3'
  rubberband_ladspa_name = 'ladspa-rubberband'
  rubberband_lv2_name = 'lv2-rubberband'
  rubberband_vamp_name = 'vamp-rubberband'
  rubberband_jni_name = 'rubberband-jni'
  unit_tests_name = 'tests'
endif  

rubberband_objlib = static_library(
  'rubberband_objlib',
  library_sources,
  feature_sources,
  include_directories: general_include_dirs,
  cpp_args: general_compile_args,
  c_args: general_compile_args,
  dependencies: general_dependencies,
  pic: true,
  install: false,
)

rubberband_objlib_dep = declare_dependency(
  link_with: rubberband_objlib,
)


# And the build targets: Static and dynamic libraries, command-line
# utility, LADSPA and LV2 plugins, Vamp plugin, JNI library

if get_option('default_library') == 'shared'
  message('Not building Rubber Band Library static library: default_library option is set to shared')
  target_summary += { 'Static library': false }
else
  message('Will build Rubber Band Library static library')
  if rubberband_additional_static_lib != ''
    target_summary += { 'Static library': [ true, 'Name: ' + rubberband_additional_static_lib ] }
  else 
    target_summary += { 'Static library': [ true, 'Name: ' + rubberband_library_name ] }
  endif
endif

if get_option('default_library') == 'static'
  message('Not building Rubber Band Library dynamic library: default_library option is set to static')
  target_summary += { 'Dynamic library': false }
else
  message('Will build Rubber Band Library dynamic library')
  target_summary += { 'Dynamic library': [ true, 'Name: ' + rubberband_library_name ] }
endif
  
rubberband_library = library(
  rubberband_library_name,
  # We would like to write "link_with: rubberband_objlib",
  # but that didn't work with MSVC when I tried it: no
  # DLL entry point found
  objects: rubberband_objlib.extract_all_objects(recursive: true),
  link_args: [
    arch_flags,
    feature_libraries,
  ],
  dependencies: general_dependencies,
  version: rubberband_dynamic_library_version,
  install: true,
)

# This dependency is not used in this build file, but is declared
# for use when including this project as a subproject using Wrap
#
rubberband_dep = declare_dependency(
  link_with: rubberband_library,
  include_directories: '.',
)

if get_option('default_library') != 'shared' and rubberband_additional_static_lib != ''
  rubberband_additional_library = static_library(
    rubberband_additional_static_lib,
    link_with: rubberband_objlib,
    name_prefix: '',
    name_suffix: 'lib',
    install: true
  )
endif

if have_jni
  target_summary += { 'JNI library': [ true, 'Name: ' + rubberband_jni_name ] }
  message('Will build Java Native Interface')
  rubberband_jni = shared_library(
    rubberband_jni_name,
    jni_sources,
    include_directories: general_include_dirs,
    cpp_args: general_compile_args,
    c_args: general_compile_args,
    link_args: [
      arch_flags,
      feature_libraries,
    ],
    dependencies: [
      rubberband_objlib_dep,
      jni_dep,
      general_dependencies,
    ],
    # NB the JNI library is not versioned
    install: true,
  )
  jar('rubberband', java_sources)
  jar('rubberband-test', java_test_sources)
else
  target_summary += { 'JNI library': false }
  if not have_java
    message('Not building Java Native Interface: Java compiler or archiver missing')
  else
    message('Not building Java Native Interface: JNI header not found')
  endif
endif

install_headers(
  public_headers,
  subdir: 'rubberband'
)

if have_ladspa
  target_summary += { 'LADSPA plugin': [ true, 'Name: ' + rubberband_ladspa_name ] }
  message('Will build LADSPA plugin')
  rubberband_ladspa = shared_library(
    rubberband_ladspa_name,
    ladspa_sources,
    include_directories: general_include_dirs,
    cpp_args: general_compile_args,
    c_args: general_compile_args,
    link_args: [
      arch_flags,
      feature_libraries,
      ladspa_symbol_args,
    ],
    dependencies: [
      rubberband_objlib_dep,
      general_dependencies,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / 'ladspa',
  )
  install_data(
    'ladspa-lv2/ladspa-rubberband.cat',
    install_dir: get_option('libdir') / 'ladspa',
  )
  install_data(
    'ladspa-lv2/ladspa-rubberband.rdf',
    install_dir: get_option('datadir') / 'ladspa/rdf',
  )
else
  target_summary += { 'LADSPA plugin': false }
  message('Not building LADSPA plugin: ladspa.h header not found')
endif

if have_lv2
  target_summary += { 'LV2 plugin': [ true, 'Name: ' + rubberband_lv2_name ] }
  message('Will build LV2 plugin')
  rubberband_lv2 = shared_library(
    rubberband_lv2_name,
    lv2_sources,
    include_directories: general_include_dirs,
    cpp_args: general_compile_args,
    c_args: general_compile_args,
    link_args: [
      arch_flags,
      feature_libraries,
      lv2_symbol_args,
    ],
    dependencies: [
      rubberband_objlib_dep,
      general_dependencies,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / 'lv2/rubberband.lv2',
  )
  install_data(
    'ladspa-lv2/rubberband.lv2/manifest.ttl',
    'ladspa-lv2/rubberband.lv2/lv2-rubberband.ttl',
    install_dir: get_option('libdir') / 'lv2/rubberband.lv2',
  )
else
  target_summary += { 'LV2 plugin': false }
  message('Not building LV2 plugin: lv2.h header not found')
endif

if have_vamp
  target_summary += { 'Vamp plugin': [ true, 'Name: ' + rubberband_vamp_name ] }
  message('Will build Vamp plugin')
  rubberband_vamp = shared_library(
    rubberband_vamp_name,
    vamp_sources,
    include_directories: general_include_dirs,
    cpp_args: general_compile_args,
    c_args: general_compile_args,
    link_args: [
      arch_flags,
      feature_libraries,
      vamp_symbol_args,
    ],
    dependencies: [
      rubberband_objlib_dep,
      general_dependencies,
      vamp_dep,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / 'vamp',
  )
  install_data(
    'vamp/vamp-rubberband.cat',
    install_dir: get_option('libdir') / 'vamp',
  )
else
  target_summary += { 'Vamp plugin': false }
  message('Not building Vamp plugin: Vamp dependency not found')
endif

if have_sndfile
  message('Will build command-line utilities')
  target_summary += { 'Command-line utility (R2)': [ true, 'Name: ' + rubberband_program_name ] }
  rubberband_program = executable(
    rubberband_program_name,
    program_sources,
    include_directories: general_include_dirs,
    cpp_args: general_compile_args,
    c_args: general_compile_args,
    link_args: [
      arch_flags,
      feature_libraries,
    ],
    dependencies: [
      rubberband_objlib_dep,
      general_dependencies,
      sndfile_dep,
    ],
    install: true,
  )
  target_summary += { 'Command-line utility (R3)': [ true, 'Name: ' + rubberband_program_name_r3 ] }
  rubberband_program_r3 = executable(
    rubberband_program_name_r3,
    program_sources,
    include_directories: general_include_dirs,
    cpp_args: general_compile_args,
    c_args: general_compile_args,
    link_args: [
      arch_flags,
      feature_libraries,
    ],
    dependencies: [
      rubberband_objlib_dep,
      general_dependencies,
      sndfile_dep
    ],
    install: true,
  )
else 
  message('Not building command-line utilities: libsndfile dependency not found')
  target_summary += { 'Command-line utility (R2)': false }
  target_summary += { 'Command-line utility (R3)': false }
endif

if have_boost_unit_test
  target_summary += { 'Unit tests': [ true, 'Name: ' + unit_tests_name ] }
  message('Will build unit tests: use "meson test -C <builddir>" to run them')
  unit_tests = executable(
    unit_tests_name,
    unit_test_sources,
    cpp_args: general_compile_args,
    c_args: general_compile_args,
    link_args: [
      arch_flags,
      feature_libraries,
    ],
    dependencies: [
      rubberband_objlib_dep,
      general_dependencies,
      boost_unit_test_dep,
    ],
    install: false,
    build_by_default: false
  )
  general_test_args = [ '--log_level=message' ]
  test('Allocators',
       unit_tests, args: [ '--run_test=TestAllocators', general_test_args ])
  test('FFT',
       unit_tests, args: [ '--run_test=TestFFT', general_test_args ])
  test('Resampler',
       unit_tests, args: [ '--run_test=TestResampler', general_test_args ])
  test('VectorOps',
       unit_tests, args: [ '--run_test=TestVectorOps', general_test_args ])
  test('VectorOpsComplex',
       unit_tests, args: [ '--run_test=TestVectorOpsComplex', general_test_args ])
  test('SignalBits',
       unit_tests, args: [ '--run_test=TestSignalBits', general_test_args ])
  test('StretchCalculator',
       unit_tests, args: [ '--run_test=TestStretchCalculator', general_test_args ])
  test('Stretcher',
       unit_tests, args: [ '--run_test=TestStretcher', general_test_args ])
else
  target_summary += { 'Unit tests': false }
  message('Not building unit tests: boost_unit_test_framework dependency not found')
endif

pkg.generate(
  name: 'rubberband',
  description: 'Audio time-stretching and pitch-shifting library',
  url: 'https://breakfastquay.com/rubberband/',
  version: meson.project_version(),
  requires: pkgconfig_requirements,
  libraries: ['-L${libdir} -lrubberband'] + pkgconfig_libraries,
  extra_cflags: '-I${includedir}',
)

summary({'prefix': get_option('prefix'),
         'bindir': get_option('bindir'),
         'libdir': get_option('libdir'),
         'datadir': get_option('datadir'),
        }, section: 'Directories')

summary(config_summary + { 'Architecture': architecture },
        section: 'Configuration', bool_yn: true)
summary(target_summary, section: 'Build targets', bool_yn: true)

if system == 'darwin'
  foreach arg: get_option('cpp_args')
    if arg.contains('iPhone')
      summary({'Please note': 'You cannot legally distribute the Rubber Band Library\n             in an iOS app on the App Store, unless you have first obtained a\n             commercial licence.'}, section: '***')
      break
    endif
  endforeach
endif
