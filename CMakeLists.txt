# SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

cmake_minimum_required(VERSION 3.24)

if(APPLE)
  # 10.15 -> std::filesystem::path
  # 11.0 -> semaphore.try_acquire
  # 13 -> https://doc-snapshots.qt.io/qt6-dev/macos.html
  # must be specified before project()
  set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum OS X deployment version")

  # this might be needed for homebrew
  if(NOT DEFINED CMAKE_OSX_SYSROOT)
    execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE CMAKE_OSX_SYSROOT OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
  message(STATUS "Using SDK: ${CMAKE_OSX_SYSROOT}")

  # universal build
  if ("${CMAKE_OSX_ARCHITECTURES}" STREQUAL "")
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
  endif()

  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++23")
  set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "NO")
elseif(MSVC)
  # Note: only DLL is allowed, and it must not be redistributed
  # See https://opensource.stackexchange.com/a/7187
  cmake_policy(SET CMP0091 NEW)  # Ensures CMAKE_MSVC_RUNTIME_LIBRARY is respected
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Git)

set(VERSION_UPDATE_FROM_GIT ${GIT_FOUND})
set(PROJECT_NAME ZRYTHM) # set this temporarily to get the version
include(GetVersionFromGitTag)

option(ZRYTHM_CCACHE "Enable ccache" OFF)
if(ZRYTHM_CCACHE)
  include(CCache)
endif()

#==============================================================================
# Project Configuration
#==============================================================================

project(zrythm
  VERSION ${ZRYTHM_VERSION_MAJOR}.${ZRYTHM_VERSION_MINOR}.${ZRYTHM_VERSION_PATCH}
  HOMEPAGE_URL "https://www.zrythm.org"
  DESCRIPTION "A highly automated and intuitive digital audio workstation (DAW)"
  LANGUAGES C CXX)

if (APPLE)
  enable_language(OBJC)
  enable_language(OBJCXX)
elseif(WIN32)
  enable_language(RC)
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_POLICY_VERSION_MINIMUM 3.24)
# Enable precompiled headers
set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)
# set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Prefer static libraries for easier linking
set(BUILD_SHARED_LIBS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/products/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/products/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/products/lib_static)

set_property(GLOBAL PROPERTY USE_FOLDERS YES)

message(STATUS "Project version: ${PROJECT_VERSION} (${ZRYTHM_VERSION_STRING_FULL})")

#==============================================================================
# Include required modules
#==============================================================================

include(GNUInstallDirs)
include(CMakeDependentOption)
include(CheckSymbolExists)
include(TestBigEndian)
include(CheckLibraryExists)
include(PreventInSourceBuilds)
include(CPM)
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)
find_package(PkgConfig)
find_package(Threads REQUIRED)

#==============================================================================
# Build/Platform-specific configuration
#==============================================================================

if(CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo|MinSizeRel")
  set(ZRYTHM_OPTIMIZED_BUILD ON)
else()
  set(ZRYTHM_OPTIMIZED_BUILD OFF)
endif()

if(UNIX AND NOT APPLE)
  set(OS_GNU ON)
else()
  set(OS_GNU OFF)
endif()

if(NOT APPLE)
  set(OS_NOT_APPLE ON)
else()
  set(OS_NOT_APPLE OFF)
endif()

if(WIN32 AND MINGW)
  set(WIN_MINGW ON)
else()
  set(WIN_MINGW OFF)
endif()

set(OPEN_DIR_CMD)
if(WIN32)
  set(OPEN_DIR_CMD "explorer.exe")
elseif(APPLE)
  set(OPEN_DIR_CMD "open")
else()
  find_program(XDG_OPEN_EXECUTABLE xdg-open REQUIRED)
  set(OPEN_DIR_CMD ${XDG_OPEN_EXECUTABLE})
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(ZRYTHM_DEV_BUILD ON)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(compiler_is_clang TRUE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(compiler_is_gcc TRUE)
endif()

if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU|Clang")
  set(compiler_gnulike TRUE)
else()
  set(compiler_gnulike FALSE)
endif()

#==============================================================================
# Options
#==============================================================================

# Options
option(ZRYTHM_TESTS "Enable/build tests" OFF)
if(ZRYTHM_TESTS)
  include(CTest)
  enable_testing()
endif()
cmake_dependent_option(ZRYTHM_BENCHMARKS "Build benchmarks" OFF "ZRYTHM_TESTS" OFF)
cmake_dependent_option(ZRYTHM_GUI_TESTS "Build GUI tests" OFF "ZRYTHM_TESTS" OFF)
option(ZRYTHM_BUNDLED_PLUGINS "Build and install bundled plugins" ON)
option(ZRYTHM_BUNDLED_PLUGINS_WITH_STATIC_LINKING "Build bundled plugins with static libs (libgcc/libstdc++). Distros should turn this off" ON)
option(ZRYTHM_MAINTAINER_MODE "Whether to enable maintainer options (only for use by Zrythm maintainers/developers)" OFF)
cmake_dependent_option(ZRYTHM_IS_INSTALLER_VER "Build installer version" OFF "ZRYTHM_MAINTAINER_MODE" OFF)
cmake_dependent_option(ZRYTHM_IS_TRIAL_VER "Build trial version" OFF "ZRYTHM_MAINTAINER_MODE" OFF)
set(ZRYTHM_PACKAGE_VERSION "Version to use/force (when making an installer package)" "${ZRYTHM_VERSION_STRING_FULL}")
option(ZRYTHM_PROFILING "Build with profiling (for gprof)" OFF)
option(ZRYTHM_MANPAGE "Build and install manpage" ${OS_GNU})
option(ZRYTHM_SHELL_COMPLETIONS "Build and install shell completions" ${UNIX})
option(ZRYTHM_WITH_JUCE_LV2_HOSTING "Include JUCE LV2 hosting module (doesn't build on MSVC)" ${UNIX})
option(ZRYTHM_USER_MANUAL "Build and install user manual" OFF)
cmake_dependent_option(ZRYTHM_STRICT_SPHINX_OPTS "Use strict sphinx options (for docs, fail on warnings)" OFF "ZRYTHM_USER_MANUAL" OFF)
option(ZRYTHM_STRICT "Compile and link with strict flags (warnings as errors, etc.)" OFF)
# Enable extra optimizations only if building with O1 or above
option(ZRYTHM_EXTRA_OPTIMIZATIONS "Compile and link with extra optimizations (for performance)" ${ZRYTHM_OPTIMIZED_BUILD})
cmake_dependent_option(ZRYTHM_EXTRA_EXTRA_OPTIMIZATIONS "Compile and link with more aggressive optimizations (experimental)" OFF "ZRYTHM_EXTRA_OPTIMIZATIONS" OFF)
cmake_dependent_option(ZRYTHM_NATIVE_OPTIMIZATIONS "Compile and link with native (-march/-mtune=native) optimizations (for performance)" OFF "ZRYTHM_EXTRA_OPTIMIZATIONS" OFF)
option(ZRYTHM_EXTRA_DEBUG_INFO "Compile and link with extra debug info (for debugging) (-g3)" OFF)
set(ZRYTHM_CARLA_BINARIES_DIR "" CACHE STRING "Location to collect carla discovery and bridge binaries")
if (WIN32)
  set(ZRYTHM_CARLA_BINARIES_32_BIT_DIR "" CACHE STRING "Location to collect carla discovery and bridge binaries for 32 bit")
endif()
option(ZRYTHM_CHECK_UPDATES "Check for updates on startup" ON)
option(ZRYTHM_IS_FLATPAK "Whether building for a Flatpak" OFF)
cmake_dependent_option(ZRYTHM_FOR_FLATHUB "Build for FlatHub (for packaging)" OFF "ZRYTHM_IS_FLATPAK" OFF)
option(ZRYTHM_RECORD_SWITCHES "Record compiler switches for debugging (if supported)" OFF)
option(ZRYTHM_ENABLE_SANITIZER_ADDRESS "Compile with address sanitizer" OFF)
option(ZRYTHM_ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "Compile with undefined behavior sanitizer" OFF)
option(ZRYTHM_ENABLE_SANITIZER_THREAD "Compile with thread sanitizer" OFF)
option(ZRYTHM_ENABLE_SANITIZER_MEMORY "Compile with memory sanitizer" OFF)
option(ZRYTHM_ENABLE_SANITIZER_REALTIME "Compile with realtime sanitizer" OFF)
option(ZRYTHM_CARLA_WITH_CV32_PATCHBAY_VARIANT "Enable support for CV32 patchbay variant in carla" ON)
option(ZRYTHM_UNITY_BUILD "Enable unity builds where possible" OFF)
option(ZRYTHM_COVERAGE "Build with --coverage" OFF)

# Dependency feature options
option(ZRYTHM_WITH_RTMIDI "Build with RtMidi support" ON)
option(ZRYTHM_WITH_RTAUDIO "Build with RtAudio support" ON)
option(ZRYTHM_WITH_JACK "Build with JACK support" OFF) # ${OS_GNU})
option(ZRYTHM_WITH_LIBDW "Build with libdw support (better backtraces)" OFF) # ${OS_GNU})
option(ZRYTHM_WITH_VALGRIND "Compile with valgrind lib (only for debugging)" OFF)

#==============================================================================
# Required Programs
#==============================================================================

find_package(Gettext)
include(FindPython3)
if (${ZRYTHM_SHELL_COMPLETIONS})
  find_program(FLEX_EXECUTABLE "flex" REQUIRED)
endif()
find_program(FAUST2LV2_EXECUTABLE "faust2lv2")
find_program(ITSTOOL_EXECUTABLE itstool)
if(APPLE)
  find_program(RSVG_CONVERT_EXECUTABLE rsvg-convert
    PATHS /opt/homebrew/Cellar/librsvg/2.60.0/bin
    REQUIRED
  )
elseif(WIN32)
  find_program(MAGICK_EXECUTABLE "magick"
    PATHS "C:/Program Files/ImageMagick-7.1.1-Q16-HDRI"
  )
endif()

#==============================================================================
# Common Variables
#==============================================================================

set(prog_name "Zrythm")
set(prog_name_lowercase "zrythm")
set(bundle_identifier "org.zrythm.Zrythm")
set(copyright_name "Alexandros Theodotou")
set(copyright_years "2018-2025")
set(main_repo_url "https://gitlab.zrythm.org/zrythm/zrythm")
set(issue_tracker_url "${main_repo_url}/-/issues")
set(chatroom_url "https://matrix.to/#/#zrythmdaw:matrix.org")
set(user_manual_url "https://manual.zrythm.org/en/index.html")
set(faq_url "https://manual.zrythm.org/en/getting-started/faq.html")
# VCS tag
set(PACKAGE_VERSION "${ZRYTHM_VERSION}") # FIXME check
# this always uses the latest release version from VERSION.txt
set(RELEASE_VERSION "${ZRYTHM_VERSION}") # FIXME check
set(zrythm_datadir "${CMAKE_INSTALL_FULL_DATADIR}/${prog_name_lowercase}")
set(themesdir "${zrythm_datadir}/themes")
set(themes_css_dir "${themesdir}/css")
set(samplesdir "${zrythm_datadir}/samples")
set(sourceviewstylesdir "${zrythm_datadir}/sourceview-styles")
set(docdir "${CMAKE_INSTALL_FULL_DATADIR}/doc/${prog_name_lowercase}")
set(zrythm_libdir "${CMAKE_INSTALL_FULL_LIBDIR}/${prog_name_lowercase}")
set(fontsdir "${CMAKE_INSTALL_FULL_DATADIR}/fonts/${prog_name_lowercase}")
set(CHECK_UPDATES ${ZRYTHM_CHECK_UPDATES})
set(FLATPAK_BUILD ${ZRYTHM_IS_FLATPAK})
set(FOR_FLATHUB ${ZRYTHM_FOR_FLATHUB})
if(ZRYTHM_IS_TRIAL_VER)
  set(TRIAL_MAX_TRACKS 25)
endif()
set(new_issue_url "${issue_tracker_url}/new")
set(privacy_policy_url "https://www.zrythm.org/en/privacy.html")
set(donation_url "https://www.zrythm.org/en/community.html#donate")
set(purchase_url "https://www.zrythm.org/en/download.html")
set(license_url "${main_repo_url}/-/raw/master/LICENSES/LicenseRef-ZrythmLicense.txt")
set(trademark_policy_url "${main_repo_url}/-/raw/master/TRADEMARKS.md")
set(bug_report_api_endpoint "https://accounts.zrythm.org/api/v1/error-reports/new")
set(PLUGIN_SCANNER_UUID "f47ac10b")
set(ZRYTHM_SVG_ICON "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/resources/icons/zrythm-dark/scalable/apps/zrythm.svg")
set(ZRYTHM_ICNS_FILE_NAME "Zrythm.icns")
set(ZRYTHM_ICNS_FILE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${ZRYTHM_ICNS_FILE_NAME}")
set(ZRYTHM_ICO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/data/platform/windows/zrythm.ico")
set(ZRYTHM_BMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/Zrythm.bmp")

# if latest version in changelog is the project's
# version, add the changelog to the config
file(READ "${CMAKE_SOURCE_DIR}/CHANGELOG.md" chlog_full)
string(REPLACE "\n## [" ";[" chlog_list "${chlog_full}")
list(LENGTH chlog_list chlog_list_len)
list(GET chlog_list 1 chlog)
string(STRIP "${chlog}" chlog)
if(chlog MATCHES "^\\[${ZRYTHM_VERSION}\\] -")
  string(REGEX MATCH " - ([^\n]+)" _ "${chlog}")
  set(chlog_date "${CMAKE_MATCH_1}")
  string(REPLACE "${chlog_date}\n" ";" chlog_list "${chlog}")
  list(GET chlog_list 1 chlog)

  set(HAVE_CHANGELOG 1)
  set(CHANGELOG_DATE "${chlog_date}")
  string(REPLACE "\"" "\\\"" chlog "${chlog}")
  string(REPLACE "\n" "\\n" CHANGELOG_TXT "${chlog}")
  # quick hack - eventually reuse the XML from appdata.xml
  string(REPLACE "\n" "</p><p>" CHANGELOG_TXT_FOR_ABOUT_DIALOG "${chlog}")
endif()

# Used for building manpages, manual, etc., using foreach.
set(language_mappings
  "af_ZA/Afrikaans"
  "ar/عربي"
  "ca/Català"
  "de/Deutsch"
  "el/Ελληνικά"
  "en/English"
  "en_GB/English UK"
  "es/Español"
  "fa/فارسی"
  "fr/Français"
  "gl/Galego"
  "he/עִבְרִית"
  "hi/हिन्दी"
  "hu/magyar nyelv"
  "id/bahasa Indonesia"
  "it/Italiano"
  "ja/日本語"
  "ko/한국어"
  "nb_NO/Bokmål"
  "mk/македонски"
  "nl/Nederlands"
  "pl/Polski"
  "pt/Português"
  "pt_BR/Português BR"
  "ru/Русский"
  "sv/Svenska"
  "th/ภาษาไทย"
  "tr/Türkiye"
  "uk/Українська"
  "vi/Tiếng Việt"
  "zh_CN/简体中文"
  "zh_TW/繁體中文"
)

foreach(lang_pair ${language_mappings})
  string(REPLACE "/" ";" lang_list ${lang_pair})
  list(GET lang_list 0 lang_code)
  list(GET lang_list 1 lang_name)
  # message(STATUS "Language ${lang_code}: ${lang_name}")
endforeach()

set(locales_list)
set(locales_list_no_en)
foreach(lang_pair ${language_mappings})
  string(REPLACE "/" ";" lang_list ${lang_pair})
  list(GET lang_list 0 lang_code)
  list(APPEND locales_list ${lang_code})
  if(NOT ${lang_code} STREQUAL "en")
    list(APPEND locales_list_no_en ${lang_code})
  endif()
endforeach()

string(JOIN " " locales_str ${locales_list})
message(STATUS "Locales: ${locales_str}")

#==============================================================================
# Required Packages
#==============================================================================

check_symbol_exists(mlock "sys/mman.h" HAVE_MLOCK)

add_library(_zrythm_include_dirs INTERFACE)
target_include_directories(_zrythm_include_dirs
  INTERFACE
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_BINARY_DIR}/src"
)
add_library(zrythm::include_dirs ALIAS _zrythm_include_dirs)

#==============================================================================
# Global Flags
#==============================================================================

# for some reason this is needed on mingw
if (${WIN_MINGW})
  # if not building an msys2 package, this might
  # also need -Wl,-allow-multiple-definition
  # see https://stackoverflow.com/questions/11267877/mingw-multiple-definition-of-unwind-resume
  add_link_options(-lssp)
endif()

if(WIN32)
  add_compile_definitions(
    _USE_MATH_DEFINES=1
    _UNICODE
    UNICODE
    NOMINMAX
  )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  # enable in case we need parallel execution
  # add_compile_definitions(-fexperimental-library)
endif()

if(OS_GNU AND compiler_gnulike)
  # drop unused library dependencies
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
endif()

add_library(_zrythm_compile_options INTERFACE)
add_library(_zrythm_warning_flags INTERFACE)
add_library(_zrythm_coverage_lib INTERFACE)

if(ZRYTHM_OPTIMIZED_BUILD)
  # LTO
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_supported)
  if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()

  if(compiler_is_gcc)
    target_compile_options(_zrythm_compile_options INTERFACE -ffunction-sections -fdata-sections)
    target_link_options(_zrythm_compile_options INTERFACE -Wl,--gc-sections)
  elseif(MSVC)
    target_link_options(_zrythm_compile_options INTERFACE /OPT:ICF)
  endif()
endif()

if(compiler_gnulike)
  target_compile_options(_zrythm_warning_flags INTERFACE -Wall -Wextra) # -Weffc++)
  if(compiler_is_clang)
    # target_compile_options(_zrythm_compile_options INTERFACE -Weverything)
  endif()
  if(ZRYTHM_COVERAGE)
    target_compile_options(_zrythm_coverage_lib INTERFACE --coverage)
    target_link_options(_zrythm_coverage_lib INTERFACE --coverage)
  endif()
elseif(MSVC)
  target_compile_options(_zrythm_warning_flags INTERFACE /W3)
endif()

# disable some warnings added by -Wextra
if(compiler_gnulike)
  target_compile_options(_zrythm_warning_flags
    INTERFACE
      -Wformat=2
      -Wno-missing-field-initializers
      -Wno-unused-parameter
      -Wno-cast-function-type
      -Wno-attributes
  )
endif()

if(ZRYTHM_RECORD_SWITCHES AND compiler_is_gcc AND NOT WIN32)
  target_compile_options(_zrythm_compile_options INTERFACE -frecord-gcc-switches)
endif()

if(ZRYTHM_NATIVE_OPTIMIZATIONS)
  if(compiler_gnulike)
    target_compile_options(_zrythm_compile_options INTERFACE -march=native -mtune=native)
  endif()
endif()

if(ZRYTHM_PROFILING)
  if(compiler_gnulike)
    target_compile_options(_zrythm_compile_options INTERFACE -pg no-pie)
  endif()
  if(${ZRYTHM_EXTRA_OPTIMIZATIONS})
    message(FATAL_ERROR "Profiling and extra optimizations are incompatible")
  endif()
endif()

if(ZRYTHM_EXTRA_OPTIMIZATIONS)
  if(compiler_gnulike)
    target_compile_options(_zrythm_compile_options INTERFACE
      -ffast-math -freciprocal-math
      # the following 2 taken from DISTRHO-Ports, need testing
      -ftree-vectorize -funroll-loops)

    # -O1 turns this on, but it may break backtraces so explicitly turn it
    # off on builds supposed to be debuggable
    # target_compile_options(_zrythm_compile_options INTERFACE -fno-omit-frame-pointer)
  elseif(MSVC)
    target_compile_options(_zrythm_compile_options INTERFACE /fp:fast)
  endif()
  if(compiler_is_gcc)
    target_compile_options(_zrythm_compile_options INTERFACE -fprefetch-loop-arrays)
  endif()
endif()

if(ZRYTHM_EXTRA_EXTRA_OPTIMIZATIONS)
  if(compiler_gnulike)
    # TODO check
    # -Ofast -fgraphite-identity -floop-nest-optimize -fdevirtualize-at-ltrans -fipa-pta -fno-semantic-interposition -flto=4 -fuse-linker-plugin -pipe -falign-functions=32 -floop-nest-optimize -floop-parallelize-all -ftree-parallelize-loops=4

    target_compile_options(_zrythm_compile_options INTERFACE
      # not allowed on clang + C
      #'-fallow-store-data-races',
      -fno-signed-zeros
      -fno-trapping-math
      -ffp-contract=fast
      -fmodulo-sched
    )
  endif()
endif()

if(ZRYTHM_EXTRA_DEBUG_INFO)
  if(compiler_gnulike)
    target_compile_options(_zrythm_compile_options INTERFACE
      -g3
      -rdynamic
      -funwind-tables
      -fasynchronous-unwind-tables
      # not supported on clang - not sure what it does either
      #'-fno-toplevel-reorder',
    )
  endif()
endif()

if(WIN32)
  if(MSVC)
    add_compile_options(/utf-8)

    target_compile_options(_zrythm_compile_options INTERFACE
      "/Zc:__cplusplus"
      # Some files are too big causing compilation errors - this increases the section limit in object files.
      # It requires slightly higher RAM but compilation time difference is negligible, so enable globally.
      /bigobj
    )
  else() # mingw
    add_compile_options(-fno-ms-extensions)
    target_compile_options(_zrythm_compile_options INTERFACE
      -mms-bitfields
      -mwindows
      #-mstackrealign
      -Wl,-Bdynamic
      -Wl,-as-needed)
  endif()
elseif(APPLE)
  target_compile_options(_zrythm_compile_options INTERFACE
    # std::jthread
    -fexperimental-library
  )
endif()

add_library(_zrythm_strict_warning_flags INTERFACE)
if(ZRYTHM_STRICT)
  if(compiler_gnulike)
    if(NOT APPLE) # apple clang doesn't understand these
      target_compile_options(_zrythm_strict_warning_flags INTERFACE
        -Werror=format-overflow
        -Werror=format-truncation
        -Werror=return-local-addr
      )
    endif()

    target_compile_options(_zrythm_strict_warning_flags INTERFACE
      -Werror=alloca
      -Werror=cast-align
      -Werror=cast-qual
      -Werror=ctor-dtor-privacy
      -Werror=deprecated
      -Werror=format-signedness
      -Wframe-larger-than=65536 # 64 KiB
      -Werror=multichar
      -Werror=non-virtual-dtor
      -Werror=range-loop-construct

      # TODO: re-enable and fix errors
      # -Werror=conversion

      -Werror=disabled-optimization
      -Werror=pointer-arith
      -Werror=enum-conversion
      -Werror=overlength-strings

      # errors on qml/intermediate files
      # -Werror=missing-declarations
      # -Werror=float-equal
      # -Werror=missing-braces

      -Werror=int-to-pointer-cast

      # maybe not needed but keep for future reference
      #-Werror=double-promotion

      -Werror=shadow
      -Werror=address
      -Werror=undef
      -Werror=unused
      -Werror=vla
      -fstrict-aliasing
      -Wstrict-aliasing=2
      -Werror=strict-aliasing
      -Werror=strict-overflow
      -Wstrict-overflow=2
      -fstrict-overflow
      -Werror=init-self
      -Werror=write-strings
      -Werror=sign-compare
      -Werror=float-conversion
      -Werror=uninitialized
      -Werror=return-type
      -Werror=switch
      -Werror=overflow
      -Werror=array-bounds
      -Werror=enum-compare
      -Werror=misleading-indentation
      -Werror=int-in-bool-context
      -Werror=type-limits
      -Werror=endif-labels
      -Werror=logical-not-parentheses
      -Werror=parentheses
      -Werror=comment
      -Werror=sizeof-pointer-div
      -Werror=shift-count-overflow
      -Werror=free-nonheap-object
      -Werror=nonnull
      -Werror=tautological-compare
      -Werror=unused-result

      # this gets triggered on asserts on gcc...
      # -Werror=unused-variable

      #-Wanalyzer-too-complex
      #-Werror=analyzer-too-complex
      -Werror=inline
      -Werror=sizeof-array-argument
      -Werror=odr
      # note: broken in gcc 14.2.1 and 15.1.1
      # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99642
      # -Werror=mismatched-tags
      -Werror=narrowing

      # these interfere with gbenchmark
      # -Werror=overloaded-virtual
      # -Werror=redundant-decls

      # disabled because it wants all class members initialized in the
      # constructor initializer list
      # -Werror=effc++

      -Werror=reorder
      -Werror=sequence-point
      -Werror=invalid-constexpr
      -Werror=pessimizing-move
      -pedantic-errors
      -Werror=pedantic
    )
    endif()
  if(compiler_is_gcc)
    target_compile_options(_zrythm_strict_warning_flags INTERFACE
      -ftemplate-backtrace-limit=0
      #-Wsuggest-attribute=pure
      #-Wsuggest-attribute=const
      #-Wsuggest-attribute=noreturn
      -Wsuggest-attribute=format
      -Wsuggest-attribute=malloc
      -Wsuggest-attribute=cold
      # only do the following on GCC because clang reports unnecessary errors
      # TODO fix and re-enable
      #-Werror=sign-conversion
      -Werror=implicit-fallthrough
      -Werror=format=2
      -Werror=endif-labels
      -Werror=clobbered
      -Werror=logical-op
      -Werror=stringop-truncation
      -Werror=duplicated-branches
      -Werror=duplicated-cond
      -Werror=maybe-uninitialized
      -Werror=multistatement-macros
      -Werror=use-after-free
      -Werror=nonnull-compare
      -Werror=bool-compare
      -Werror=lto-type-mismatch
      -Werror=dangling-reference
      -Werror=dangling-pointer
      -Werror=missing-template-keyword
      # -Werror=noexcept # TODO: investigate further
      -Wformat-overflow=2

      # -fanalyzer
      -Werror=analyzer-possible-null-dereference
      # false positives with GCC
      #-Werror=analyzer-malloc-leak
      # -Werror=analyzer-null-dereference
      -Werror=analyzer-null-argument
      -Werror=analyzer-use-after-free
      -Werror=analyzer-possible-null-argument
      -Werror=analyzer-double-free
      # false positives with GCC
      #-Werror=analyzer-file-leak
      # GCC still makes mistakes
      #-Werror=analyzer-use-of-uninitialized-value
      -Werror=analyzer-out-of-bounds
      -Werror=analyzer-deref-before-check
      -Werror=analyzer-allocation-size
      -Werror=analyzer-exposure-through-uninit-copy
      #-Werror=analyzer-imprecise-fp-arithmetic
      -Werror=analyzer-infinite-recursion
      -Werror=analyzer-jump-through-null
      -Werror=analyzer-fd-leak
    )
  elseif(compiler_is_clang)
    target_compile_options(_zrythm_strict_warning_flags INTERFACE
      -Werror=vla-cxx-extension
      -Werror=sentinel
      -Werror=return-stack-address
      -Werror=abstract-final-class
      -Werror=null-dereference
      -Werror=defaulted-function-deleted
      -Wexit-time-destructors
      -Wglobal-constructors
      -fsafe-buffer-usage-suggestions

      # For RealtimeSanitizer compile-time checks
      # TODO: enable and convert to -Werror=
      # -Wfunction-effects
      -Wperf-constraint-implies-noexcept
    )
  elseif(MSVC)
  endif()
endif()
add_library(zrythm::coverage ALIAS _zrythm_coverage_lib)
add_library(zrythm::compile_options ALIAS _zrythm_compile_options)
add_library(zrythm::warning_flags ALIAS _zrythm_warning_flags)
add_library(zrythm::strict_warning_flags ALIAS _zrythm_strict_warning_flags)
add_library(_zrythm_all_compile_options INTERFACE)
target_link_libraries(_zrythm_all_compile_options INTERFACE
  zrythm::compile_options
  zrythm::warning_flags
  zrythm::strict_warning_flags
  zrythm::coverage
)
add_library(zrythm::all_compile_options ALIAS _zrythm_all_compile_options)

# Sanitizers
add_library(_sanitizers_lib INTERFACE)
include(Sanitizers)
enable_sanitizers(_sanitizers_lib
  ZRYTHM_ENABLE_SANITIZER_ADDRESS
  ZRYTHM_ENABLE_SANITIZER_LEAK
  ZRYTHM_ENABLE_SANITIZER_UNDEFINED_BEHAVIOR
  ZRYTHM_ENABLE_SANITIZER_THREAD
  ZRYTHM_ENABLE_SANITIZER_MEMORY
  ZRYTHM_ENABLE_SANITIZER_REALTIME
)
add_library(zrythm::sanitizers ALIAS _sanitizers_lib)

##############################################################
# More dependencies
##############################################################

CPMFindPackage(
  NAME nlohmann_json
  GITHUB_REPOSITORY nlohmann/json
  VERSION 3.12.1
  # 3.12.0 + some commits later
  GIT_TAG 230bfd15a2bb7f01ebb3fcd3cf898b697ef43c48
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
  OPTIONS
    "JSON_ImplicitConversions OFF"
    "JSON_SystemInclude ON"
)

CPMFindPackage(
  NAME clap
  GITHUB_REPOSITORY free-audio/clap
  VERSION 1.2.6
  GIT_TAG 69a69252fdd6ac1d06e246d9a04c0a89d9607a17
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
)
CPMFindPackage(
  NAME clap-helpers
  GITHUB_REPOSITORY free-audio/clap-helpers
  VERSION 0
  GIT_TAG 4d8c96fd7da2a7a67a91cf87b008643d6ad2c9ba
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
)

add_subdirectory(ext)

CPMFindPackage(
  NAME SndFile
  GITHUB_REPOSITORY libsndfile/libsndfile
  VERSION 1.2.2
  GIT_TAG 72f6af15e8f85157bd622ed45b979025828b7001
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
  OPTIONS
    "BUILD_EXAMPLES OFF"
    "BUILD_TESTING OFF"
    "ENABLE_EXTERNAL_LIBS ON"
    "ENABLE_CPACK OFF"
    "ENABLE_PACKAGE_CONFIG OFF"
    "INSTALL_PKGCONFIG_MODULE OFF"
    "INSTALL_MANPAGES OFF"
)
set(HAVE_OPUS TRUE)

# add_library(lv2_lib INTERFACE)
# target_include_directories(lv2_lib
  # INTERFACE ${juce_SOURCE_DIR}/modules/juce_audio_processors/format_types/LV2_SDK/lv2)
# pkg_check_modules(LV2 lv2>=1.18.0 REQUIRED IMPORTED_TARGET)
# add_library(zrythm::lv2 ALIAS PkgConfig::LV2)
# pkg_check_modules(ZIX zix-0>=0.4.0 REQUIRED IMPORTED_TARGET)
# pkg_check_modules(SERD serd-0>=0.30.0 REQUIRED IMPORTED_TARGET)
# pkg_check_modules(SORD sord-0>=0.14.0 REQUIRED IMPORTED_TARGET)
# pkg_check_modules(SRATOM sratom-0>=0.4.0 REQUIRED IMPORTED_TARGET)
# pkg_check_modules(LILV lilv-0>=0.24.6 REQUIRED IMPORTED_TARGET)

pkg_check_modules(CHROMAPRINT libchromaprint IMPORTED_TARGET)
add_library(_zrythm_chromaprint_lib INTERFACE)
if(CHROMAPRINT_FOUND AND FALSE) # disable for now
  set(HAVE_CHROMAPRINT TRUE)
  target_link_libraries(_zrythm_chromaprint_lib INTERFACE PkgConfig::CHROMAPRINT)
endif()
add_library(zrythm::chromaprint_lib ALIAS _zrythm_chromaprint_lib)

# Carla
if(false)
  pkg_check_modules(CARLA carla-host-plugin>=2.6.0 REQUIRED IMPORTED_TARGET)
  set(HAVE_CARLA TRUE)
  if(ZRYTHM_CARLA_BINARIES_DIR)
    string(REPLACE "\\" "/" ZRYTHM_CARLA_BINARIES_DIR ${ZRYTHM_CARLA_BINARIES_DIR})
  else()
    pkg_get_variable(carla_lib_dir carla-host-plugin "carla_libdir")
    set(ZRYTHM_CARLA_BINARIES_DIR ${carla_lib_dir})
  endif()
  find_program(CARLA_DISCOVERY_NATIVE_EXECUTABLE
    NAMES "carla-discovery-native" "carla-discovery-native.exe"
    PATHS "${ZRYTHM_CARLA_BINARIES_DIR}"
    REQUIRED)
  set(zrythm_carla_libdir "${zrythm_libdir}/carla")
  install(PROGRAMS ${CARLA_DISCOVERY_NATIVE_EXECUTABLE}
    DESTINATION "${zrythm_carla_libdir}")
  set(CARLA_HAVE_CLAP_SUPPORT TRUE)
  set(CARLA_HAVE_CV32_PATCHBAY_VARIANT ${ZRYTHM_CARLA_WITH_CV32_PATCHBAY_VARIANT})

  # install gnu/linux bridges
  set(bridge_types
    native lv2-gtk2 lv2-gtk3 lv2-qt4 lv2-qt5 lv2-x11)
  foreach(bridge_type ${bridge_types})
    find_program(CARLA_BRIDGE_${bridge_type}
      NAMES "carla-bridge-${bridge_type}"
      PATHS "${ZRYTHM_CARLA_BINARIES_DIR}"
      NO_DEFAULT_PATH)
    if(CARLA_BRIDGE_${bridge_type} AND (OS_GNU))
      string(TOUPPER ${bridge_type} bridge_type_upper)
      string(REPLACE "-" "_" bridge_type_upper ${bridge_type_upper})
      set(HAVE_CARLA_BRIDGE_${bridge_type_upper} 1)
      install(PROGRAMS ${CARLA_BRIDGE_${bridge_type}}
        DESTINATION "${zrythm_libdir}/carla")
    endif()
  endforeach()
  set(CMAKE_REQUIRED_LIBRARIES ${CARLA_LIBRARIES})
  check_symbol_exists(carla_get_native_patchbay_cv8_plugin "carla/CarlaHost.h" CARLA_HAVE_CV8_PATCHBAY_VARIANT)
  check_symbol_exists(carla_get_audio_port_hints "carla/CarlaHost.h" CARLA_HAVE_AUDIO_PORT_HINTS)
  set(CMAKE_REQUIRED_LIBRARIES)
endif()
set(HAVE_CARLA FALSE)

CPMFindPackage(
  NAME zstd
  GITHUB_REPOSITORY facebook/zstd
  VERSION 1.5.7
  GIT_TAG f8745da6ff1ad1e7bab384bd1f9d742439278e99
  SOURCE_SUBDIR build/cmake
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
  OPTIONS
    "ZSTD_BUILD_STATIC ON"
    "ZSTD_BUILD_SHARED OFF"
    "ZSTD_LEGACY_SUPPORT OFF"
    "ZSTD_BUILD_PROGRAMS OFF"
    "ZSTD_BUILD_CONTRIB OFF"
    "ZSTD_BUILD_TESTS OFF"
)
add_library(_zstd_lib INTERFACE)
target_link_libraries(_zstd_lib INTERFACE libzstd_static)
add_library(zrythm::zstd ALIAS _zstd_lib)

CPMFindPackage(
  NAME xxHash
  GITHUB_REPOSITORY Cyan4973/xxHash
  VERSION 0.8.2
  GIT_TAG bbb27a5efb85b92a0486cf361a8635715a53f6ba
  SOURCE_SUBDIR cmake_unofficial
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
  OPTIONS
    "XXHASH_BUILD_XXHSUM OFF"
)

# not used, might use in the future
if(0)
  pkg_check_modules(VAMP_SDK vamp-sdk IMPORTED_TARGET)
  pkg_check_modules(VAMP_HOST_SDK vamp-hostsdk IMPORTED_TARGET)
  if((WIN32 AND NOT MSVC) OR (NOT VAMP_FOUND) OR (NOT VAMP_HOST_SDK_FOUND))
    FetchContent_Declare(
      vamp-sdk
      GIT_REPOSITORY https://github.com/vamp-plugins/vamp-plugin-sdk.git
      # master on 2024-10-01
      GIT_TAG 703438d8e55dbfd1c2021d0bc12e36edd1e227e2
      GIT_SHALLOW TRUE
      EXCLUDE_FROM_ALL
      SYSTEM
    )
    FetchContent_MakeAvailable(vamp-sdk)
  else()
    add_library(vamp-plugin-sdk::vamp-hostsdk ALIAS PkgConfig::VAMP_HOST_SDK)
    add_library(vamp-plugin-sdk::vamp-sdk ALIAS PkgConfig::VAMP_SDK)
  endif()
endif()

# magic_enum
CPMFindPackage(
  NAME magic_enum
  GITHUB_REPOSITORY Neargye/magic_enum
  VERSION 0.9.6
  GIT_TAG dd6a39d0ba1852cf06907e0f0573a2a10d23c2ad
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
)

# fmt
CPMFindPackage(
  NAME fmt
  GITHUB_REPOSITORY fmtlib/fmt
  VERSION 11.2.0
  GIT_TAG 40626af88bd7df9a5fb80be7b25ac85b122d6c21
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
  OPTIONS
    "FMT_INSTALL OFF"
)

# scnlib
CPMFindPackage(
  NAME scnlib
  GITHUB_REPOSITORY eliaskosunen/scnlib
  VERSION 4.0.1
  GIT_TAG e937be1a52588621b406d58ce8614f96bb5de747
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
)

# spdlog
CPMFindPackage(
  NAME spdlog
  GITHUB_REPOSITORY gabime/spdlog
  VERSION 1.15.3
  GIT_TAG 6fa36017cfd5731d617e1a934f0e5ea9c4445b13
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
  OPTIONS
    "SPDLOG_INSTALL OFF"
)
add_library(_spdlog_lib INTERFACE)
target_link_libraries(_spdlog_lib INTERFACE spdlog::spdlog)
target_compile_definitions(_spdlog_lib INTERFACE
  SPDLOG_FMT_EXTERNAL
  SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_definitions(_spdlog_lib INTERFACE
    SPDLOG_FUNCTION=__PRETTY_FUNCTION__
  )
else()
  target_compile_definitions(_spdlog_lib INTERFACE
    SPDLOG_FUNCTION=__FUNCTION__
  )
endif()
add_library(zrythm::spdlog ALIAS _spdlog_lib)

# pkg_check_modules(RUBBERBAND rubberband REQUIRED IMPORTED_TARGET)

FetchContent_Declare(
  Boost
  GIT_REPOSITORY https://github.com/boostorg/boost
  # 1.88.0
  GIT_TAG 199ef13d6034c85232431130142159af3adfce22
  GIT_SHALLOW TRUE
  GIT_SUBMODULES
    libs/assert
    libs/circular_buffer
    libs/concept_check
    libs/config
    libs/container
    libs/container_hash
    libs/core
    libs/describe
    libs/intrusive
    libs/move
    libs/mp11
    libs/predef
    libs/preprocessor
    libs/static_assert
    libs/stl_interfaces
    libs/throw_exception
    libs/type_traits
    libs/unordered
    tools/cmake
  EXCLUDE_FROM_ALL
  SYSTEM
)
set(BOOST_INCLUDE_LIBRARIES
  circular_buffer
  container
  describe
  stl_interfaces
  unordered
)
FetchContent_MakeAvailable(Boost)

CPMFindPackage(
  NAME type_safe
  GITHUB_REPOSITORY foonathan/type_safe
  VERSION 0.2.4
  GIT_TAG 1dbea7936a1e389caa692d01bb3f5ba4b6da7d82
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
)

if(0)
  # getting compiler errors so unused for now
  FetchContent_Declare(
    mpunits
    GIT_REPOSITORY https://github.com/mpusz/mp-units
    # master 2025/05/30
    GIT_TAG 514abd2140bcd3ba310c4295cbf247874d838673
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR src
    EXCLUDE_FROM_ALL
    SYSTEM
  )
  set(MP_UNITS_BUILD_AS_SYSTEM_HEADERS ON)
  set(MP_UNITS_API_STD_FORMAT OFF)
  set(MP_UNITS_API_NO_CRTP OFF)
  set(MP_UNITS_API_FREESTANDING ON)
  set(MP_UNITS_API_CONTRACTS NONE)
  set(MP_UNITS_BUILD_CXX_MODULES OFF)
  FetchContent_MakeAvailable(mpunits)
  # mp-units::mp-units
endif()

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
CPMFindPackage(
  NAME GTest
  GITHUB_REPOSITORY google/googletest
  VERSION 1.16.0
  GIT_TAG 6910c9d9165801d8827d628cb72eb7ea9dd538c5
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
  OPTIONS
    "INSTALL_GTEST OFF"
)
add_library(_gtest_libs INTERFACE)
target_link_libraries(_gtest_libs INTERFACE gtest)
add_library(_gtest_main_libs INTERFACE)
target_link_libraries(_gtest_main_libs INTERFACE gtest_main gmock_main)
add_library(zrythm::gtest ALIAS _gtest_libs)
add_library(zrythm::gtest_for_tests ALIAS _gtest_main_libs)

CPMFindPackage(
  NAME benchmark
  GITHUB_REPOSITORY google/benchmark
  VERSION 1.9.2
  GIT_TAG afa23b7699c17f1e26c88cbf95257b20d78d6247
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
  OPTIONS
    "BENCHMARK_ENABLE_TESTING OFF"
    "BENCHMARK_ENABLE_INSTALL OFF"
    "BENCHMARK_INSTALL_DOCS OFF"
    "CMAKE_BUILD_TYPE Release"
)
add_library(_benchmark_libs INTERFACE)
target_link_libraries(_benchmark_libs INTERFACE benchmark::benchmark_main gmock)
if(MSVC)
  target_compile_options(benchmark PRIVATE /wd4267)
elseif(compiler_gnulike)
  target_compile_options(_benchmark_libs INTERFACE -Wno-error=overloaded-virtual -Wno-error=redundant-decls)
endif()
add_library(zrythm::benchmark ALIAS _benchmark_libs)

CPMFindPackage(
  NAME gsl-lite
  GITHUB_REPOSITORY gsl-lite/gsl-lite
  VERSION 1.0.1
  GIT_TAG 56dab5ce071c4ca17d3e0dbbda9a94bd5a1cbca1
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
)

set(tbb_opts "")
list(APPEND tbb_opts "TBB_STRICT OFF")
list(APPEND tbb_opts "TBB_TEST OFF")
list(APPEND tbb_opts "TBB_INSTALL OFF")
list(APPEND tbb_opts "TBBMALLOC_BUILD OFF")
list(APPEND tbb_opts "TBBMALLOC_PROXY_BUILD OFF")
if(ZRYTHM_ENABLE_SANITIZER_ADDRESS)
  list(APPEND tbb_opts "TBB_SANITIZE address")
elseif(ZRYTHM_ENABLE_SANITIZER_THREAD)
  list(APPEND tbb_opts "TBB_SANITIZE thread")
elseif(ZRYTHM_ENABLE_SANITIZER_MEMORY)
  list(APPEND tbb_opts "TBB_SANITIZE memory")
endif()
CPMFindPackage(
  NAME tbb
  GITHUB_REPOSITORY "zrythm/oneTBB"
  VERSION 2022.2.0
  GIT_TAG fe153f04aebb223d500f27bc3f93374a012273ec
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
  OPTIONS ${tbb_opts}
)

if(ZRYTHM_TESTS)
  CPMFindPackage(
    NAME cpp_httplib
    GITHUB_REPOSITORY yhirose/cpp-httplib
    VERSION 0.20.1
    GIT_TAG 3af7f2c16147f3fbc6e4d717032daf505dc1652c
    OPTIONS
      "HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF"
      "HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF"
      "HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF"
      "HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF"
      "HTTPLIB_INSTALL OFF"
  )
endif()

CPMFindPackage(
  NAME farbot
  GITHUB_REPOSITORY "zrythm/farbot"
  VERSION 0.1
  GIT_TAG 2a2bd99eefada6c78dff04bc0692912366952a86
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
)

find_package(Qt6
  COMPONENTS
    Core
    Gui
    Widgets
    Quick
    QuickControls2
    Concurrent
    LinguistTools
    Test
  REQUIRED
)
qt_standard_project_setup(
  I18N_TRANSLATED_LANGUAGES ${locales_list_no_en}
)
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)
qt_policy(SET QTP0005 NEW)
target_compile_definitions(Qt6::Core INTERFACE
  "QT_ENABLE_STRICT_MODE_UP_TO=0x060900"
  "QT_NO_KEYWORDS"
)
get_filename_component(QT6_CMAKE_DIR "${Qt6_DIR}" DIRECTORY)
get_filename_component(QT6_PREFIX_PATH "${QT6_CMAKE_DIR}/../.." ABSOLUTE)
if(WIN32)
  find_program(WINDEPLOYQT_EXECUTABLE windeployqt REQUIRED PATHS "${QT6_PREFIX_PATH}/bin" NO_DEFAULT_PATH)
elseif(APPLE)
  find_program(MACDEPLOYQT_EXECUTABLE macdeployqt REQUIRED PATHS "${QT6_PREFIX_PATH}/bin" NO_DEFAULT_PATH)
endif()

add_library(qt_libs INTERFACE)
target_link_libraries(qt_libs
  INTERFACE
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Quick
  Qt6::QuickControls2
  # for qmltc
  # Qt6::QmlPrivate
  # Qt6::QuickPrivate
  Qt6::Concurrent
)
add_library(zrythm::qt_libs ALIAS qt_libs)

list(APPEND zrythm_link_libs
  zita-resampler::zita-resampler
  zrythm::soxr
  # PkgConfig::CARLA
  # PkgConfig::RTAUDIO
  # PkgConfig::RTMIDI
  zrythm::zstd
  zrythm::chromaprint_lib
  Threads::Threads
  zrythm::rubberband
  ${CMAKE_DL_LIBS}
  xxHash::xxhash
  nlohmann_json::nlohmann_json
  magic_enum::magic_enum
  zrythm::juce_libs
  backward_cpp::backward_cpp
  crill::crill
  fmt::fmt
  scn::scn
  zrythm::spdlog
  type_safe
  midilib::midilib
  kissfft::kissfft
  Boost::describe
  Boost::stl_interfaces
  Boost::container
  Boost::unordered
  gsl-lite::gsl-lite
  clap-helpers
  TBB::tbb
  farbot::farbot
  zrythm::qt_libs
  zrythm::gtest
  zrythm::sanitizers
  zrythm::lightweight_semaphore
)

if(ZRYTHM_WITH_JACK)
  include(FindJACK)
  list(APPEND zrythm_link_libs zrythm::jack)
endif()

if(APPLE)
  find_library(FOUNDATION_LIBRARY Foundation REQUIRED)
  find_library(COCOA_LIBRARY Cocoa REQUIRED)
  find_library(APPKIT_LIBRARY AppKit REQUIRED)
  add_library(zrythm_apple_libs INTERFACE)
  target_link_libraries(zrythm_apple_libs
    INTERFACE
      ${FOUNDATION_LIBRARY}
      ${COCOA_LIBRARY}
      ${APPKIT_LIBRARY}
  )
  list(APPEND zrythm_link_libs zrythm_apple_libs)
endif()

file(COPY
  AUTHORS
  TRANSLATORS
  THANKS
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

configure_file(
  src/zrythm-config.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/zrythm-config.h)

# Plugins for testing
set(ext_lv2_plugins
  "ams_lfo|AMS LFO|http://github.com/blablack/ams-lv2/lfo"
  "calf_monosynth|Calf Monosynth|http://calf.sourceforge.net/plugins/Monosynth"
  "helm|Helm|http://tytel.org/helm"
  "sherlock_atom_inspector|Sherlock Atom Inspector|http://open-music-kontrollers.ch/lv2/sherlock#atom_inspector"
  "lsp_compressor_mono|LSP Compressor Mono|http://lsp-plug.in/plugins/lv2/compressor_mono"
  "lsp_compressor|LSP Compressor|http://lsp-plug.in/plugins/lv2/compressor_stereo"
  "lsp_sidechain_compressor|LSP Sidechain Compressor|http://lsp-plug.in/plugins/lv2/sc_compressor_stereo"
  "lsp_multisampler_24_do|LSP MultiSampler x24 Direct Out|http://lsp-plug.in/plugins/lv2/multisampler_x24_do"
  "carla_rack|Carla Rack|http://kxstudio.sf.net/carla/plugins/carlarack"
  "no_delay_line|No Delay Line|http://gareus.org/oss/lv2/nodelay"
  "mda_ambience|mda Ambience|http://drobilla.net/plugins/mda/Ambience"
  "midi_cc_map|MIDI CC Map|http://gareus.org/oss/lv2/midifilter#mapcc"
  "noize_maker|NoizeMak3r|http://kunz.corrupt.ch/products/tal-noisemaker"
  "tal_filter|TAL Filter|urn:juce:TalFilter"
  "geonkick|Geonkick|http://geontime.com/geonkick/single"
  "chipwave|ChipWave|https://github.com/linuxmao-org/shiru-plugins/chipwave"
  "calf_compressor|Calf Compressor|http://calf.sourceforge.net/plugins/Compressor"
  "mverb|MVerb|http://distrho.sf.net/plugins/MVerb"
  "sfizz|Sfizz|http://sfztools.github.io/sfizz"
  "drops|Drops|http://github.com/clearly-broken-software/drops"
  "test_signal|Test Signal|http://gareus.org/oss/lv2/testsignal"
  "kxstudio_lfo|KXStudio LFO|http://kxstudio.sf.net/carla/plugins/lfo")

set(ext_vst_plugins
  "noizemaker|TAL-NoiseMaker.so"
)

set(ext_vst3_plugins
  "onetrick_simian|Punk Labs LLC OneTrick SIMIAN.vst3"
  "dexed|Dexed.vst3"
  "surge_xt|Surge XT.vst3"
)

# When using the lilv subproject, we can't use lv2ls and lv2info
find_program(LV2INFO_EXECUTABLE lv2info)

if(ZRYTHM_TESTS)
  find_program(LV2LS_EXECUTABLE lv2ls)

  # Get LV2 bundle paths
  if(LV2LS_EXECUTABLE AND LV2INFO_EXECUTABLE)
    execute_process(
      COMMAND ${LV2LS_EXECUTABLE}
      OUTPUT_VARIABLE lv2ls_res
      RESULT_VARIABLE lv2ls_result
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(lv2ls_result EQUAL 0)
      foreach(plugin_info ${ext_lv2_plugins})
        string(REPLACE "|" ";" plugin_info "${plugin_info}")
        list(GET plugin_info 0 name)
        list(GET plugin_info 2 uri)

        if(lv2ls_res MATCHES "${uri}")
          set(have_ext_lv2_plugins_${name} TRUE)

          execute_process(
            COMMAND ${LV2INFO_EXECUTABLE} ${uri}
            OUTPUT_VARIABLE lv2info_output
            RESULT_VARIABLE lv2info_result
            OUTPUT_STRIP_TRAILING_WHITESPACE
          )

          if(lv2info_result EQUAL 0)
            string(REGEX MATCH "Bundle:[^\n]*" bundle_line "${lv2info_output}")
            if(bundle_line)
              string(REGEX REPLACE "Bundle:[ \t]*" "" bundle_uri "${bundle_line}")
              set(ext_lv2_plugin_bundles_${name} "${bundle_uri}")
            else()
              message(WARNING "lv2info failed to find bundle uri for ${name}")
            endif()
          endif()
        endif()
      endforeach()
    endif()
  endif()

  # Get VST plugin paths
  find_program(GET_VST_PATH_EXECUTABLE get_vst_path.sh PATH tools)
  if(GET_VST_PATH_EXECUTABLE)
    foreach(plugin_info ${ext_vst_plugins})
      string(REPLACE "|" ";" plugin_info "${plugin_info}")
      list(GET plugin_info 0 name)
      list(GET plugin_info 1 filename)

      execute_process(
        COMMAND ${GET_VST_PATH_EXECUTABLE} ${filename}
        RESULT_VARIABLE vst_result
        OUTPUT_VARIABLE vst_path
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )

      if(vst_result EQUAL 0)
        set(have_ext_vst_plugins_${name} TRUE)
        set(ext_vst_plugin_paths_${name} "${vst_path}")
      endif()
    endforeach()

    foreach(plugin_info ${ext_vst3_plugins})
      string(REPLACE "|" ";" plugin_info "${plugin_info}")
      list(GET plugin_info 0 name)
      list(GET plugin_info 1 filename)

      execute_process(
        COMMAND ${GET_VST_PATH_EXECUTABLE} "${filename}" 3
        RESULT_VARIABLE vst3_result
        OUTPUT_VARIABLE vst3_path
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )

      if(vst3_result EQUAL 0)
        set(have_ext_vst3_plugins_${name} TRUE)
        set(ext_vst3_plugin_paths_${name} "${vst3_path}")
      endif()
    endforeach()
  endif()
endif()

# Add subdirectories
add_subdirectory(data)
add_subdirectory(src)

# below errors out on Xcode
if(NOT XCODE)
  set(QT_NO_MISSING_CATALOG_LANGUAGE_WARNING ON)
  qt_add_translations(zrythm
    TS_FILE_DIR i18n
    MERGE_QT_TRANSLATIONS
    QT_TRANSLATION_CATALOGS qtbase
  )
endif()

# clang-format integration
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_custom_target(clang-format-check
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/run_clang_format.py --dry-run
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-format check..."
)
add_custom_target(clang-format
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/run_clang_format.py
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Running clang-format..."
)

if(ZRYTHM_TESTS)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.cmake ${CMAKE_CURRENT_BINARY_DIR} @ONLY)
  add_subdirectory(tests)
endif()
add_subdirectory(doc)

#==============================================================================
# CPack / Packaging
#==============================================================================

if(WIN32)
  install(CODE "
    execute_process(
      COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
        --qmldir \"${CMAKE_BINARY_DIR}\"
        --force
        # Distributing the VC redistributable violates the GPL
        # See: https://www.gnu.org/licenses/gpl-faq.html#RedistributingWindows
        --no-compiler-runtime
        --no-system-d3d-compiler
        --no-system-dxc-compiler
        $<$<CONFIG:Debug>:--pdb>
        -qml -widgets -quick -quickwidgets
        \"\$\{CMAKE_INSTALL_PREFIX\}/bin/$<TARGET_FILE_NAME:zrythm>\"
      COMMAND_ERROR_IS_FATAL ANY
    )
  "
  COMPONENT Runtime)
elseif(APPLE)
  install(CODE "
    set(ZRYTHM_APP_BUNDLE_DIR \"\$\{CMAKE_INSTALL_PREFIX\}/$<TARGET_BUNDLE_DIR_NAME:zrythm>\")

    # Deploy Qt dependencies
    execute_process(
      COMMAND \"${MACDEPLOYQT_EXECUTABLE}\"
        \"\$\{ZRYTHM_APP_BUNDLE_DIR\}\"
        -qmldir=${CMAKE_BINARY_DIR}
        -always-overwrite
        \"-executable=\$\{ZRYTHM_APP_BUNDLE_DIR\}/Contents/MacOS/$<TARGET_FILE_NAME:plugin-scanner>\"
      COMMAND_ERROR_IS_FATAL ANY
    )

    # Code sign the app bundle
    # No need to notarize since we'll notarize the .dmg later, and it covers
    # the .app too
    execute_process(
      COMMAND codesign
        --deep --force --verbose $<$<CONFIG:Release>:--options;runtime> $<$<CONFIG:Release>:--timestamp>
        --entitlements \"${CMAKE_SOURCE_DIR}/data/platform/macos/Zrythm.entitlements\"
        --sign \"${APPLE_APP_CODESIGN_IDENTITY}\"
        \"\$\{ZRYTHM_APP_BUNDLE_DIR\}\"
      COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
      COMMAND codesign
      --verify --deep --verbose
      \"\$\{ZRYTHM_APP_BUNDLE_DIR\}\"
      COMMAND_ERROR_IS_FATAL ANY
    )
  "
  COMPONENT Runtime)
else()
  install(
    IMPORTED_RUNTIME_ARTIFACTS
      Qt6::Core
      Qt6::Gui
      Qt6::Widgets
      Qt6::Quick
      Qt6::QuickControls2
      Qt6::Concurrent
    RUNTIME
      COMPONENT Runtime
    LIBRARY
      COMPONENT Runtime
      NAMELINK_COMPONENT Development
  )
  install(
    DIRECTORY ${QT6_PREFIX_PATH}/plugins/
    DESTINATION plugins
    COMPONENT Runtime
  )
  install(
    DIRECTORY ${QT6_PREFIX_PATH}/lib/
    TYPE LIB
    COMPONENT Runtime
    FILES_MATCHING
    PATTERN "*.so*"
    PATTERN "cmake" EXCLUDE
    PATTERN "pkgconfig" EXCLUDE
    PATTERN "objects-*" EXCLUDE
  )
  install(
    DIRECTORY ${QT6_PREFIX_PATH}/qml/
    DESTINATION qml
    COMPONENT Runtime
    PATTERN "*"
    PATTERN "Qt5Compat" EXCLUDE
    PATTERN "objects-*" EXCLUDE
    PATTERN "*.a" EXCLUDE
    PATTERN "*.prl" EXCLUDE
  )
  if(compiler_is_gcc)
    # bundle libstdc++
    execute_process(
      COMMAND sh -c "gcc --print-file-name=libstdc++.so"
      OUTPUT_VARIABLE _libstdcxx_path
      OUTPUT_STRIP_TRAILING_WHITESPACE
      COMMAND_ERROR_IS_FATAL ANY
    )
    get_filename_component(_libstdcxx_path "${_libstdcxx_path}" REALPATH)
    install(
      FILES "${_libstdcxx_path}"
      DESTINATION lib
      COMPONENT Runtime
      RENAME "libstdc++.so.6"
    )
  endif()
endif()

# Post-install script
install(SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/cmake/post_install.cmake)

set(CPACK_PACKAGE_NAME "Zrythm")
set(CPACK_PACKAGE_VENDOR "Alexandros Theodotou and Contributors")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Zrythm ${ZRYTHM_VERSION_MAJOR}.${ZRYTHM_VERSION_MINOR}")
set(CPACK_PACKAGE_CHECKSUM "MD5")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSES/LicenseRef-ZrythmLicense.txt")
set(CPACK_PACKAGE_EXECUTABLES "zrythm;Zrythm")
set(CPACK_THREADS 0) # use all CPU cores

# cpack_add_component_group(Zrythm
#   DISPLAY_NAME "Zrythm"
#   DESCRIPTION "Zrythm"
#   EXPANDED
#   BOLD_TITLE
# )
cpack_add_component(Runtime
  DISPLAY_NAME "Zrythm Runtime"
  DESCRIPTION "Zrythm Runtime"
  REQUIRED
  # GROUP Zrythm
)
set(CPACK_COMPONENTS_ALL "Runtime")

if(WIN32)
  if(MAGICK_EXECUTABLE)
    # Installer banner
    set(CPACK_PACKAGE_ICON "${ZRYTHM_BMP_FILE}")
  endif()
elseif(APPLE)
  set(CPACK_SYSTEM_NAME "MacOS")
  set(CPACK_PACKAGE_ICON "${ZRYTHM_ICNS_FILE_PATH}")
endif()

# -- Generator-specific options --

# Archive generator (GNU/Linux)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

# DragNDrop generator (MacOS)
set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE ON)
set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_SOURCE_DIR}/data/platform/macos/dmg_setup.scpt")
set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/data/platform/macos/dmg_background_512.png")

# INNO Setup generator (Windows)
set(CPACK_INNOSETUP_USE_MODERN_WIZARD ON)
set(CPACK_INNOSETUP_ICON_FILE "${ZRYTHM_ICO_FILE}")
set(CPACK_INNOSETUP_LANGUAGES "english;brazilianPortuguese;catalan;french;german;hebrew;italian;japanese;norwegian;polish;portuguese;russian;spanish;turkish;ukrainian")
set(CPACK_INNOSETUP_RUN_EXECUTABLES "zrythm")

include(CPack) # must be included after specifying the CPACK_ variables
