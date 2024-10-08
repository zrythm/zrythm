# SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

cmake_minimum_required(VERSION 3.24)

if(APPLE)
  # 10.15 -> std::filesystem::path
  # 11.0 -> semaphore.try_acquire
  # must be specified before project()
  set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version")
  
  # this might be needed for homebrew
  if(NOT DEFINED CMAKE_OSX_SYSROOT)
    execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE CMAKE_OSX_SYSROOT OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
  message(STATUS "Using SDK: ${CMAKE_OSX_SYSROOT}")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Git)

set(VERSION_UPDATE_FROM_GIT ${GIT_FOUND})
set(PROJECT_NAME ZRYTHM) # set this temporarily to get the version
include(GetVersionFromGitTag)

#==============================================================================
# Project Configuration
#==============================================================================

project(zrythm
  VERSION ${ZRYTHM_VERSION_MAJOR}.${ZRYTHM_VERSION_MINOR}.${ZRYTHM_VERSION_PATCH}
  HOMEPAGE_URL "https://www.zrythm.org"
  DESCRIPTION "Highly automated and intuitive digital audio workstation"
  LANGUAGES C CXX)

if (APPLE)
  enable_language(OBJC)
  enable_language(OBJCXX)
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++20")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Enable precompiled headers
set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)

# Set the release runtime library for all targets to make our life easier
# (all dependencies must use the same runtime library, so build all
# dependencies against the release runtime library first)
# Note: only DLL is allowed, and it must not be redistributed
# See https://opensource.stackexchange.com/a/7187
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

message(STATUS "Project version: ${PROJECT_VERSION} (${ZRYTHM_VERSION_STRING_FULL})")

#==============================================================================
# Include required modules
#==============================================================================

include(GNUInstallDirs)
include(CMakeDependentOption)
include(CheckSymbolExists)
include(TestBigEndian)
include(FetchContent)
include(CheckLibraryExists)
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)
find_package(PkgConfig REQUIRED)
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

if(CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo")
  set(ZRYTHM_DEBUG_BUILD ON)
else()
  set(ZRYTHM_DEBUG_BUILD OFF)
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
  enable_testing()
endif()
cmake_dependent_option(ZRYTHM_BENCHMARKS "Build benchmarks" OFF "ZRYTHM_TESTS" OFF)
cmake_dependent_option(ZRYTHM_GUI_TESTS "Build GUI tests" OFF "ZRYTHM_TESTS" OFF)
option(ZRYTHM_BUNDLED_PLUGINS "Build and install bundled plugins" ON)
option(ZRYTHM_BUNDLED_PLUGINS_WITH_STATIC_LINKING "Build bundled plugins with static libs (libgcc/libstdc++). Distros should turn this off" ON)
option(ZRYTHM_DSEG_FONT "Install the DSEG14 font (packagers should turn this off and set the font as a dependency instead)" ON)
option(ZRYTHM_MAINTAINER_MODE "Whether to enable maintainer options (only for use by Zrythm maintainers/developers)" OFF)
cmake_dependent_option(ZRYTHM_IS_INSTALLER_VER "Build installer version" OFF "ZRYTHM_MAINTAINER_MODE" OFF)
cmake_dependent_option(ZRYTHM_IS_TRIAL_VER "Build trial version" OFF "ZRYTHM_MAINTAINER_MODE" OFF)
set(ZRYTHM_PACKAGE_VERSION "Version to use/force (when making an installer package)" "${ZRYTHM_VERSION_STRING_FULL}")
option(ZRYTHM_PROFILING "Build with profiling (for gprof)" OFF)
option(ZRYTHM_MANPAGE "Build and install manpage" ${OS_GNU})
option(ZRYTHM_SHELL_COMPLETIONS "Build and install shell completions" ${UNIX})
option(ZRYTHM_USER_MANUAL "Build and install user manual" OFF)
cmake_dependent_option(ZRYTHM_STRICT_SPHINX_OPTS "Use strict sphinx options (for docs, fail on warnings)" OFF "ZRYTHM_USER_MANUAL" OFF)
option(ZRYTHM_STRICT "Compile and link with strict flags (warnings as errors, etc.)" OFF)
# Enable extra optimizations only if building with O1 or above
option(ZRYTHM_EXTRA_OPTIMIZATIONS "Compile and link with extra optimizations (for performance)" OFF)
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
option(ZRYTHM_YAML_PROJECT_COMPATIBILITY "Compatibility with old CYAML-based projects (requires libcyaml)." ON)
option(ZRYTHM_RECORD_SWITCHES "Record compiler switches for debugging (if supported)" OFF)
# Whether to look for fftw3_threads pkgconfig files (or libs directly if not found) separate from the main fftw3 one
option(ZRYTHM_FFTW3_THREADS_SEPARATE "Look for fftw3_threads separately" ON)
set(ZRYTHM_FFTW3_THREADS_SEPARATE_TYPE "auto" CACHE STRING "Method to look for separate fftw3_threads")
set_property(CACHE ZRYTHM_FFTW3_THREADS_SEPARATE_TYPE PROPERTY STRINGS auto dependency library)
option(ZRYTHM_FFTW3F_SEPARATE "Look for fftw3f separately of fftw3" ON)
option(ZRYTHM_ENABLE_SANITIZER_ADDRESS "Compile with address sanitizer" OFF)
option(ZRYTHM_ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "Compile with undefined behavior sanitizer" OFF)
option(ZRYTHM_ENABLE_SANITIZER_THREAD "Compile with thread sanitizer" OFF)
option(ZRYTHM_ENABLE_SANITIZER_MEMORY "Compile with memory sanitizer" OFF)
option(ZRYTHM_CARLA_WITH_CV32_PATCHBAY_VARIANT "Enable support for CV32 patchbay variant in carla" ON)

# Dependency feature options
option(ZRYTHM_WITH_X11 "Build with X11 support" ${OS_GNU})
option(ZRYTHM_WITH_RTMIDI "Build with RtMidi support" ON)
option(ZRYTHM_WITH_RTAUDIO "Build with RtAudio support" ON)
option(ZRYTHM_WITH_JACK "Build with JACK support" ${OS_GNU})
option(ZRYTHM_WITH_LSP_DSP "Build with LSP DSP for optimized float math" ON)
option(ZRYTHM_WITH_VALGRIND "Compile with valgrind lib (only for debugging)" OFF)

#==============================================================================
# Required Programs
#==============================================================================

find_package(Gettext REQUIRED)
include(FindPython3)
if (${ZRYTHM_SHELL_COMPLETIONS})
  find_program(FLEX_EXECUTABLE "flex" REQUIRED)
endif()
find_program(XMLLINT_EXECUTABLE "xmllint" REQUIRED)
find_program(FAUST2LV2_EXECUTABLE "faust2lv2")
find_program(ITSTOOL_EXECUTABLE itstool)

#==============================================================================
# Common Variables
#==============================================================================

set(prog_name "Zrythm")
set(prog_name_lowercase "zrythm")
set(copyright_name "The Zrythm contributors")
set(copyright_years "2018-2024")
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
set(gschemas_dir "${CMAKE_INSTALL_FULL_DATADIR}/glib-2.0/schemas")
set(fontsdir "${CMAKE_INSTALL_FULL_DATADIR}/fonts/${prog_name_lowercase}")
set(IS_DEBUG_BUILD ${ZRYTHM_DEBUG_BUILD})
set(CHECK_UPDATES ${ZRYTHM_CHECK_UPDATES})
set(FLATPAK_BUILD ${ZRYTHM_IS_FLATPAK})
set(FOR_FLATHUB ${ZRYTHM_FOR_FLATHUB})
set(HAVE_BUNDLED_DSEG ${ZRYTHM_DSEG_FONT})
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
  "ar/اَلْعَرَبِيَّةُ‎"
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
foreach(lang_pair ${language_mappings})
  string(REPLACE "/" ";" lang_list ${lang_pair})
  list(GET lang_list 0 lang_code)
  list(APPEND locales_list ${lang_code})
endforeach()

string(JOIN " " locales_str ${locales_list})
message(STATUS "Locales: ${locales_str}")

if(${compiler_gnulike})
  set(full_optimization_args "-O3")
endif()

#==============================================================================
# Required Packages
#==============================================================================

set(gtk_req "4.14.0")
set(glib_req "2.74.0")

check_symbol_exists(mlock "sys/mman.h" HAVE_MLOCK)
message(STATUS "HAVE_MLOCK: ${HAVE_MLOCK}")

set(zrythm_include_dirs
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/ext"
  "${CMAKE_BINARY_DIR}/src"
  )

if(${ZRYTHM_WITH_JACK})
  if (WIN32 OR ${ZRYTHM_IS_INSTALLER_VER})
    set(USE_WEAK_JACK ON)
  else()
    set(USE_WEAK_JACK OFF)
  endif()

  # prefer jack1
  pkg_check_modules(JACK IMPORTED_TARGET jack<1.0)
  if(NOT JACK_FOUND)
    pkg_check_modules(JACK REQUIRED IMPORTED_TARGET jack>=1.0)
    set(HAVE_JACK2 ON)
  endif()
  set(HAVE_JACK ON)

  if(USE_WEAK_JACK)
    list(APPEND zrythm_include_dirs ${JACK_INCLUDE_DIRS})
  else()
    list(APPEND zrythm_link_libs PkgConfig::JACK)
  endif()

  set(CMAKE_REQUIRED_INCLUDES ${JACK_INCLUDE_DIRS})
  set(CMAKE_REQUIRED_LIBRARIES ${JACK_LIBRARIES})
  check_library_exists("${JACK_LIBRARIES}" jack_set_property "" HAVE_JACK_SET_PROPERTY)
  check_library_exists("${JACK_LIBRARIES}" jack_port_type_get_buffer_size "" HAVE_JACK_PORT_TYPE_GET_BUFFER_SIZE)
endif()

# Glib/GTK-related compile definitions
set(gnome_compile_definitions)
string(REPLACE "." ";" glib_version_arr ${glib_req})
list(GET glib_version_arr 0 glib_major_version)
list(GET glib_version_arr 1 glib_minor_version)
string(REPLACE "." ";" gtk_version_arr ${gtk_req})
list(GET gtk_version_arr 0 gtk_major_version)
list(GET gtk_version_arr 1 gtk_minor_version)
list(APPEND gnome_compile_definitions
    GLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_${glib_major_version}_${glib_minor_version}
    GLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_${glib_major_version}_${glib_minor_version}
    GDK_VERSION_MIN_REQUIRED=GDK_VERSION_${gtk_major_version}_${gtk_minor_version}
    GDK_VERSION_MAX_ALLOWED=GDK_VERSION_${gtk_major_version}_14
)

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

# Zrythm-specific compile definitions
set(zrythm_definitions)
if(${compiler_is_clang})
  list(APPEND zrythm_definitions "ATTR_REALTIME=__attribute__((annotate(\"realtime\")))")
else()
  list(APPEND zrythm_definitions "ATTR_REALTIME=")
endif()
list(APPEND zrythm_definitions GETTEXT_PACKAGE="zrythm")

if(WIN32)
  add_compile_definitions(
    _USE_MATH_DEFINES=1
    _UNICODE
    UNICODE
    NOMINMAX
  )
endif()

set(attribute_macros
  cold hot nonnull returns_nonnull const
)
foreach(a ${attribute_macros})
  string(TOUPPER ${a} a_upper)
  set(attribute_def "ATTR_${a_upper}=")
  if(MSVC)
    # Only support specific attributes on MSVC (if any)
  elseif(${compiler_gnulike})
    set(attribute_def "${attribute_def}__attribute__((${a}))")
  endif()
  list(APPEND zrythm_definitions ${attribute_def})
endforeach()

# compile args
set(zrythm_compile_args)
if(compiler_gnulike)
  list(APPEND zrythm_compile_args -Wall -Wextra
  #  -Weffc++
  )
  if(compiler_is_clang)
    # list(APPEND zrythm_compile_args -Weverything)
  endif()
elseif(MSVC)
  list(APPEND zrythm_compile_args /W3)
endif()

# disable some warnings added by -Wextra
if(${compiler_gnulike})
  list(APPEND zrythm_compile_args
    -Wformat=2
    -Wno-missing-field-initializers
    -Wno-unused-parameter
    -Wno-cast-function-type)
endif()

if(ZRYTHM_RECORD_SWITCHES AND compiler_is_gcc AND NOT WIN32)
  list(APPEND zrythm_compile_args -frecord-gcc-switches)
endif()

if(ZRYTHM_NATIVE_OPTIMIZATIONS)
  if(${compiler_gnulike})
    list(APPEND zrythm_compile_args -march=native -mtune=native)
  endif()
endif()

if(ZRYTHM_PROFILING)
  if(${compiler_gnulike})
    list(APPEND zrythm_compile_args -pg no-pie)
  endif()
  if(${ZRYTHM_EXTRA_OPTIMIZATIONS})
    message(FATAL_ERROR "Profiling and extra optimizations are incompatible")
  endif()
endif()

if(ZRYTHM_EXTRA_OPTIMIZATIONS)
  if(${compiler_gnulike})
    list(APPEND zrythm_compile_args
      -ffast-math -DPIC -freciprocal-math
      -fsingle-precision-constant
      # the following 2 taken from DISTRHO-Ports, need testing
      -ftree-vectorize -funroll-loops)
    if(NOT APPLE)
      list(APPEND zrythm_compile_args -fprefetch-loop-arrays)
    endif()

    if(IS_DEBUG_BUILD)
      # -O1 turns this on, but it may break backtraces so explicitly turn it
      # off on builds supposed to be debuggable
      list(APPEND zrythm_compile_args -fno-omit-frame-pointer)
    endif()
  endif()
endif()

if(ZRYTHM_EXTRA_EXTRA_OPTIMIZATIONS)
  if(${compiler_gnulike})
    # TODO check
    # -Ofast -fgraphite-identity -floop-nest-optimize -fdevirtualize-at-ltrans -fipa-pta -fno-semantic-interposition -flto=4 -fuse-linker-plugin -pipe -falign-functions=32 -floop-nest-optimize -floop-parallelize-all -ftree-parallelize-loops=4

    list(APPEND zrythm_compile_args
      # not allowed on clang + C
      #'-fallow-store-data-races',
      -ffinite-math-only
      -fno-signed-zeros
      -fno-trapping-math
      -ffp-contract=fast
      -fmodulo-sched
    )
  endif()
endif()

if(ZRYTHM_EXTRA_DEBUG_INFO)
  if(compiler_gnulike)
    list(APPEND zrythm_compile_args
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
    list(APPEND zrythm_compile_args "/Zc:__cplusplus")
  else() # mingw
    add_compile_options(-fno-ms-extensions)
    list(APPEND zrythm_compile_args
      -mms-bitfields
      -mwindows
      #-mstackrealign
      -Wl,-Bdynamic
      -Wl,-as-needed)
  endif()
endif()

if(ZRYTHM_STRICT)
  if(compiler_gnulike)
    if(NOT APPLE) # apple clang doesn't understand these
      list(APPEND zrythm_compile_args
        -Werror=format-overflow
        -Werror=format-truncation
        -Werror=return-local-addr
      )
    endif()

    list(APPEND zrythm_compile_args
      -Werror=alloca
      #-Werror=cast-qual
      # FIXME enable and fix all reported errors
      #-Werror=conversion

      -Werror=disabled-optimization
      -Werror=float-equal
      -Werror=pointer-arith
      -Werror=enum-conversion
      # this interferes with gresources generated C file
      # and I don't know how to turn this off just for that file
      # -Werror=overlength-strings
      -Werror=missing-declarations
      # -Werror=int-to-pointer-cast

      # maybe not needed but keep for future reference
      #-Werror=double-promotion

      #-Werror=redundant-decls
      -Werror=shadow
      -Werror=address
      -Werror=undef
      -Werror=unused
      -Werror=vla
      -fstrict-aliasing
      -Wstrict-aliasing=2
      -Werror=strict-aliasing
      #-Werror=strict-overflow
      -Wstrict-overflow=2
      -fstrict-overflow
      -Werror=init-self
      -Werror=write-strings
      # TODO fix and enable
      #-Werror=sign-compare
      -Werror=float-conversion
      -Werror=uninitialized
      -Werror=return-type
      -Werror=switch
      -Werror=overflow
      # breaks pango subproject build
      #-Werror=array-bounds
      -Werror=enum-compare
      -Werror=misleading-indentation
      -Werror=int-in-bool-context
      -Werror=type-limits
      # TODO re-enable after porting gtk widgets
      #-Werror=deprecated-declarations
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
      -Werror=unused-variable
      #-Wanalyzer-too-complex
      #-Werror=analyzer-too-complex
      -Werror=inline
      -Werror=redundant-decls
      -Werror=sizeof-array-argument
      -Werror=odr
      -Werror=mismatched-tags
      -Werror=narrowing
      -Werror=overloaded-virtual
      # -Werror=effc++
      -Werror=reorder
      -Werror=sequence-point
    )
    endif()
  if(compiler_is_gcc)
    list(APPEND zrythm_compile_args
      #-Wsuggest-attribute=pure
      #-Wsuggest-attribute=const
      #-Wsuggest-attribute=noreturn
      -Wsuggest-attribute=format
      #-Wsuggest-attribute=malloc
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
    list(APPEND zrythm_compile_args
      -Werror=vla-cxx-extension
      -Werror=sentinel
      -Werror=return-stack-address
      -Werror=abstract-final-class
      -Werror=null-dereference
      -fsafe-buffer-usage-suggestions
    )
  endif()
endif()

# Linker libs/flags
set(zrythm_link_args)
if(WIN32)
  # use MgwHelp if found, otherwise regular DbgHelp
  find_library(DBGHELP_LIB NAMES mgwhelp DbgHelp REQUIRED)
  list(APPEND zrythm_link_libs ${DBGHELP_LIB} ws2_32 winmm)

  if (NOT MSVC)
    list(APPEND zrythm_link_args
      -fno-stack-protector
      -static-libgcc
      -static-libstdc++)
  endif()
endif()

# Sanitizers
if(compiler_gnulike)
  set(sanitizers)
  if(ZRYTHM_ENABLE_SANITIZER_ADDRESS)
    list(APPEND sanitizers "address")
    add_compile_options(
      -fsanitize-address-use-after-scope
    )
  endif()
  if(ZRYTHM_ENABLE_SANITIZER_UNDEFINED_BEHAVIOR)
    list(APPEND sanitizers "undefined")
  endif()
  if(ZRYTHM_ENABLE_SANITIZER_THREAD)
    if("address" IN_LIST sanitizers OR "undefined" IN_LIST sanitizers)
      message(FATAL_ERROR "Address/undefined sanitizers cannot be enabled together with thread sanitizer")
    endif()
    list(APPEND sanitizers "thread")
  endif()
  if(ZRYTHM_ENABLE_SANITIZER_MEMORY)
    list(APPEND sanitizers "memory")
    add_compile_options(
      # "-fsanitize-blacklist=${CMAKE_SOURCE_DIR}/tools/san-blacklist.txt"
      -fsanitize-memory-track-origins
    )
  endif()

  list(JOIN sanitizers "," sanitizers_list)
  if(sanitizers_list)
    if(NOT "${sanitizers_list}" STREQUAL "")
      add_compile_options(
        -fsanitize=${sanitizers_list}
        -fno-omit-frame-pointer
        -fno-optimize-sibling-calls
      )
      add_link_options(
        -fsanitize=${sanitizers_list}
      )
    endif()
  endif()
endif()

##############################################################
# More dependencies
##############################################################

FetchContent_Declare(
  yyjson
  GIT_REPOSITORY https://github.com/ibireme/yyjson.git
  # 0.10.0
  GIT_TAG 9ddba001a4ea88e93b46932e5c5b87b222e19a5f
  FIND_PACKAGE_ARGS NAMES yyjson
)
FetchContent_MakeAvailable(yyjson)
if(NOT TARGET yyjson::yyjson)
  add_library(yyjson::yyjson ALIAS yyjson)
endif()

add_subdirectory(ext)

if(ZRYTHM_WITH_X11 AND NOT APPLE)
  find_package(X11 REQUIRED)
  set(HAVE_X11 TRUE)
endif()

if(ZRYTHM_YAML_PROJECT_COMPATIBILITY)
  pkg_check_modules(LIBCYAML libcyaml>=1.2.0 IMPORTED_TARGET)
  if(LIBCYAML_FOUND)
    add_library(cyaml::cyaml ALIAS PkgConfig::LIBCYAML)
  else()
    message(WARNING "libcyaml not found, falling back to bundled libcyaml")
    FetchContent_Declare(
      libcyaml
      GIT_REPOSITORY https://gitlab.zrythm.org/zrythm/zrythm-libcyaml.git
      GIT_TAG 6cae0db42e28900a9c5bbba11816fb1c5caf0f7b
    )
    FetchContent_MakeAvailable(libcyaml)
  endif()
  set(HAVE_CYAML TRUE)
endif()

find_package(SndFile 1.0.29 REQUIRED)
set(HAVE_OPUS TRUE)

find_package(soxr QUIET)
if(NOT soxr_FOUND)
  pkg_check_modules(SOXR soxr IMPORTED_TARGET)
  if(SOXR_FOUND)
    add_library(soxr::soxr ALIAS PkgConfig::SOXR)
  else()
    find_library(SOXR_LIB soxr
      PATHS ${SOXR_LIB_SEARCH_DIR}
      REQUIRED
    )
    find_path(SOXR_INCLUDE_DIR soxr.h
      PATHS ${SOXR_INCLUDE_DIR_SEARCH_DIR}
      REQUIRED
    )
    add_library(soxr::soxr UNKNOWN IMPORTED)
    set_target_properties(soxr::soxr PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${SOXR_INCLUDE_DIR}"
      IMPORTED_LOCATION "${SOXR_LIB}"
    )
  endif()
endif()

# link to fontconfig/pangoft2 directly because
# they are used in utils/pango.c
pkg_check_modules(FONTCONFIG fontconfig>=2.13.0 REQUIRED IMPORTED_TARGET)
pkg_check_modules(PANGOFT2 pangoft2>=1.50 REQUIRED IMPORTED_TARGET)

# include separately with -isystem to bypass
# https://github.com/ebassi/graphene/issues/249
# NOTE this won't fix the issue when using graphene as a subproject
pkg_check_modules(GRAPHENE graphene-1.0 REQUIRED IMPORTED_TARGET)

pkg_check_modules(GTK4 "gtk4>=${gtk_req}" REQUIRED IMPORTED_TARGET)

pkg_check_modules(LIBADWAITA libadwaita-1>=1.5 REQUIRED IMPORTED_TARGET)
pkg_check_modules(LIBPANEL libpanel-1>=1.0.1 REQUIRED IMPORTED_TARGET)

pkg_check_modules(LV2 lv2>=1.16.0 REQUIRED IMPORTED_TARGET)
if(${LV2_VERSION} VERSION_GREATER_EQUAL 1.18.0)
  set(HAVE_LV2_1_18 TRUE)
endif()
# pkg_check_modules(ZIX zix-0>=0.4.0 REQUIRED IMPORTED_TARGET)
# pkg_check_modules(SERD serd-0>=0.30.0 REQUIRED IMPORTED_TARGET)
# pkg_check_modules(SORD sord-0>=0.14.0 REQUIRED IMPORTED_TARGET)
# pkg_check_modules(SRATOM sratom-0>=0.4.0 REQUIRED IMPORTED_TARGET)
pkg_check_modules(LILV lilv-0>=0.24.6 REQUIRED IMPORTED_TARGET)

include(FindFFTW3)

pkg_check_modules(CHROMAPRINT libchromaprint IMPORTED_TARGET)
if(CHROMAPRINT_FOUND)
  set(HAVE_CHROMAPRINT TRUE)
endif()

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

pkg_search_module(RTAUDIO librtaudio>=5.1.0 rtaudio>=5.1.0 REQUIRED IMPORTED_TARGET)
set(HAVE_RTAUDIO TRUE)
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)" _ ${RTAUDIO_VERSION})
set(RTAUDIO_MAJOR_VER ${CMAKE_MATCH_1})
set(RTAUDIO_MINOR_VER ${CMAKE_MATCH_2})
pkg_search_module(RTMIDI librtmidi>=5.0.0 rtmidi>=5.0.0 REQUIRED IMPORTED_TARGET)
if(RTMIDI_VERSION VERSION_GREATER_EQUAL 6.0.0)
  set(HAVE_RTMIDI_6 TRUE)
endif()

find_package(zstd REQUIRED)

pkg_check_modules(LSP_DSP_LIB lsp-dsp-lib>=0.5.5 IMPORTED_TARGET)
if(LSP_DSP_LIB_FOUND)
  set(HAVE_LSP_DSP TRUE)
endif()

pkg_check_modules(XXHASH libxxhash REQUIRED IMPORTED_TARGET)

pkg_check_modules(VAMP_SDK vamp-sdk IMPORTED_TARGET)
pkg_check_modules(VAMP_HOST_SDK vamp-hostsdk IMPORTED_TARGET)
if((WIN32 AND NOT MSVC) OR (NOT VAMP_FOUND) OR (NOT VAMP_HOST_SDK_FOUND))
  FetchContent_Declare(
    vamp-sdk
    GIT_REPOSITORY https://github.com/vamp-plugins/vamp-plugin-sdk.git
    # master on 2024-10-01
    GIT_TAG 703438d8e55dbfd1c2021d0bc12e36edd1e227e2
  )
  FetchContent_MakeAvailable(vamp-sdk)
else()
  add_library(vamp-plugin-sdk::vamp-hostsdk ALIAS PkgConfig::VAMP_HOST_SDK)
  add_library(vamp-plugin-sdk::vamp-sdk ALIAS PkgConfig::VAMP_SDK)
endif()

FetchContent_Declare(
  magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  # 0.9.6
  GIT_TAG dd6a39d0ba1852cf06907e0f0573a2a10d23c2ad
)
find_package(magic_enum 0.9.5)
if(NOT magic_enum_FOUND)
  FetchContent_MakeAvailable(magic_enum)
endif()

find_package(fmt REQUIRED)

find_package(spdlog REQUIRED)
target_compile_definitions(spdlog::spdlog INTERFACE
  SPDLOG_FMT_EXTERNAL
  SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_definitions(spdlog::spdlog INTERFACE
    SPDLOG_FUNCTION=__PRETTY_FUNCTION__
  )
else()
  target_compile_definitions(spdlog::spdlog INTERFACE
    SPDLOG_FUNCTION=__FUNCTION__
  )
endif()

pkg_check_modules(GLIBMM glibmm-2.68 REQUIRED IMPORTED_TARGET)
pkg_check_modules(GIOMM giomm-2.68 REQUIRED IMPORTED_TARGET)
pkg_check_modules(GTKMM gtkmm-4.0 REQUIRED IMPORTED_TARGET)

pkg_check_modules(RUBBERBAND rubberband REQUIRED IMPORTED_TARGET)

pkg_check_modules(EPOXY epoxy REQUIRED IMPORTED_TARGET)

find_package(GTest REQUIRED)

find_package(Qt6
  COMPONENTS Core Gui Widgets Quick QuickControls2 Concurrent REQUIRED
)
qt_standard_project_setup()

qt_add_executable(zrythm WIN32 MACOSX_BUNDLE)

# set(KDDockWidgets_QT6 TRUE)
# find_package(KDDockWidgets-qt6 REQUIRED)

# find_package(KF6Kirigami REQUIRED)

list(APPEND zrythm_link_libs
  zita-resampler::zita-resampler
  PkgConfig::LIBADWAITA
  PkgConfig::LIBPANEL
  PkgConfig::FONTCONFIG
  PkgConfig::PANGOFT2
  PkgConfig::GRAPHENE
  PkgConfig::GTK4
  SndFile::sndfile
  cyaml::cyaml
  soxr::soxr
  # PkgConfig::CARLA
  PkgConfig::RTAUDIO
  PkgConfig::RTMIDI
  zstd::libzstd_shared
  Threads::Threads
  PkgConfig::RUBBERBAND
  PkgConfig::EPOXY
  ${CMAKE_DL_LIBS}
  FFTW3::FFTW3
  PkgConfig::XXHASH
  vamp-plugin-sdk::vamp-sdk
  vamp-plugin-sdk::vamp-hostsdk
  yyjson::yyjson
  magic_enum::magic_enum
  juce_lib
  juce_clap_hosting_lib
  backward_cpp::backward_cpp
  fmt::fmt
  spdlog::spdlog
  PkgConfig::GLIBMM
  PkgConfig::GIOMM
  PkgConfig::GTKMM
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Quick
  Qt6::QuickControls2
  # for qmltc
  Qt6::QmlPrivate
  Qt6::QuickPrivate
  Qt6::Concurrent
  GTest::GTest
)

if(CHROMAPRINT_FOUND)
  list(APPEND zrythm_link_libs PkgConfig::CHROMAPRINT)
endif()
if(LSP_DSP_LIB_FOUND)
  list(APPEND zrythm_link_libs PkgConfig::LSP_DSP_LIB)
endif()

if(OS_GNU)
  find_library(LIBRT rt REQUIRED)
  list(APPEND zrythm_link_libs ${LIBRT})
elseif(APPLE)
  find_library(FOUNDATION_LIBRARY Foundation REQUIRED)
  find_library(COCOA_LIBRARY Cocoa REQUIRED)
  find_library(APPKIT_LIBRARY AppKit REQUIRED)
  list(APPEND zrythm_link_libs ${FOUNDATION_LIBRARY} ${COCOA_LIBRARY} ${APPKIT_LIBRARY})
endif()

if(ZRYTHM_WITH_X11)
  list(APPEND zrythm_link_libs X11::X11)
endif()

file(COPY
  HACKING.md
  AUTHORS
  TRANSLATORS
  THANKS
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

configure_file(
  src/zrythm-config.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/zrythm-config.h)

# Plugins for testing
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
add_subdirectory(po)
add_subdirectory(data)
add_subdirectory(src)

# ==============================================================================================
# QML setup (must be toplevel for some reason)
# ==============================================================================================

# module ends up in "/<RESOURCE_PREFIX>/<URI>/"
qt_add_qml_module(zrythm
  URI Zrythm
  VERSION 1.0
  RESOURCE_PREFIX /org.zrythm/imports
  QML_FILES 
    ${qml_file_paths}
    ${CMAKE_CURRENT_BINARY_DIR}/src/gui/js/config.js
  RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/resources/qtquickcontrols2.conf
    ${icon_files}
    ${font_files}
  SOURCES # for some reason singletons must be added here
    src/global_state.h
    src/global_state.cpp
  
  # NO_RESOURCE_TARGET_PATH

  # compile qml files listed in QML_FILES to C++ using qmltc
  # (currently cannot extend QtQuick Controls directly)
  # ENABLE_TYPE_COMPILER
)

# ==============================================================================================

message(STATUS "Zrythm compile definitions: ${zrythm_definitions}")
message(STATUS "Zrythm compile args: ${zrythm_compile_args}")
message(STATUS "Zrythm link libs: ${zrythm_link_libs}")
message(STATUS "Zrythm link args: ${zrythm_link_args}")

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
  add_subdirectory(tests)
endif()
add_subdirectory(doc)

# Post-install script
install(SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/cmake/post_install.cmake)
