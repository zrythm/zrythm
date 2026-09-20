# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# LV2 test fixtures: real C plugins written against the LV2 spec headers.
# Each fixture builds a self-contained bundle under
# <ZRYTHM_LV2_FIXTURE_ROOT>/$<CONFIG>/<name>.lv2/ (manifest, ttl and the
# plugin binary side by side, as a real .lv2 bundle is laid out).

# add_lv2_fixture(<target> <bundle-dir-name> <binary-name>
#   [NO_MANIFEST] [TTL_FILES <ttl...>] [SOURCES <sources...>]
#   [DATA_FILES <files...>])
#
# SOURCES may be omitted for ttl-only bundles (e.g. presets), which contain no
# plugin library at all. NO_MANIFEST skips the manifest/ttl staging for
# additional libraries in an already-staged bundle (e.g. a plugin's UI).
# DATA_FILES stage bundle-local data files verbatim.
function(add_lv2_fixture target bundle_dir output_name)
  cmake_parse_arguments(ARG "NO_MANIFEST" "" "TTL_FILES;SOURCES;DATA_FILES" ${ARGN})
  set(bundle_root ${ZRYTHM_LV2_FIXTURE_ROOT}/$<CONFIG>/${bundle_dir})

  # The bundle path contains $<CONFIG>, which configure_file() cannot emit,
  # so files are staged with configure_file() and then generated into the
  # bundle with file(GENERATE)
  if(NOT ARG_NO_MANIFEST)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${bundle_dir}/manifest.ttl.in)
      set(manifest_src ${CMAKE_CURRENT_SOURCE_DIR}/${bundle_dir}/manifest.ttl.in)
    else()
      set(manifest_src ${CMAKE_CURRENT_SOURCE_DIR}/${bundle_dir}/manifest.ttl)
    endif()
    configure_file(${manifest_src} ${bundle_dir}.manifest.ttl.staged @ONLY)
    file(READ ${CMAKE_CURRENT_BINARY_DIR}/${bundle_dir}.manifest.ttl.staged manifest_content)
    file(GENERATE OUTPUT ${bundle_root}/manifest.ttl CONTENT "${manifest_content}")

    foreach(ttl_file ${ARG_TTL_FILES})
      # a .ttl.in source is configured like the manifest (substituting
      # @VARS@) and staged under the plain .ttl name
      if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${bundle_dir}/${ttl_file}.in)
        set(ttl_src ${CMAKE_CURRENT_SOURCE_DIR}/${bundle_dir}/${ttl_file}.in)
      else()
        set(ttl_src ${CMAKE_CURRENT_SOURCE_DIR}/${bundle_dir}/${ttl_file})
      endif()
      configure_file(${ttl_src} ${bundle_dir}.${ttl_file}.staged @ONLY)
      file(READ ${CMAKE_CURRENT_BINARY_DIR}/${bundle_dir}.${ttl_file}.staged ttl_content)
      file(GENERATE OUTPUT ${bundle_root}/${ttl_file} CONTENT "${ttl_content}")
    endforeach()
  endif()

  # Data files (e.g. the sample the eg-sampler fixture loads) are staged
  # verbatim; unlike the ttl staging above they hold binary content, so
  # they go through file(GENERATE)'s INPUT form
  foreach(data_file ${ARG_DATA_FILES})
    file(GENERATE
      OUTPUT ${bundle_root}/${data_file}
      INPUT ${CMAKE_CURRENT_SOURCE_DIR}/${bundle_dir}/${data_file})
  endforeach()

  if(NOT ARG_SOURCES)
    return()
  endif()

  add_library(${target} MODULE ${ARG_SOURCES})
  # MSVC needs C11 atomics enabled explicitly for <stdatomic.h> in C
  if(MSVC)
    target_compile_options(${target} PRIVATE /experimental:c11atomics)
  endif()
  # Sanitizers (when enabled) let violations inside the fixture be caught;
  # the fixtures are only ever loaded by sanitizer-enabled test binaries
  target_link_libraries(${target} PRIVATE lv2::lv2 zrythm::sanitizers)
  set_target_properties(${target} PROPERTIES
    PREFIX ""
    # The manifests reference @CMAKE_SHARED_LIBRARY_SUFFIX@, which differs
    # from the module suffix CMake picks on macOS (.so)
    SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}"
    OUTPUT_NAME ${output_name}
    LIBRARY_OUTPUT_DIRECTORY ${bundle_root}
    UNITY_BUILD OFF
    AUTOMOC OFF
  )
  zrythm_target_disable_warnings(${target})
endfunction()

add_lv2_fixture(eg_amp_lv2 eg-amp.lv2 amp
  TTL_FILES amp.ttl amp-ui.ttl amp-float-window.ttl presets.ttl
  SOURCES eg-amp.lv2/amp.c)
# stub X11UI for the eg-amp plugin (live UI session tests); not a plugin,
# so it is not part of the fixture plugin target list, but it must still
# build with the tests: the UI session tests dlopen it
add_lv2_fixture(eg_amp_ui_lv2 eg-amp.lv2 amp-ui
  NO_MANIFEST
  SOURCES eg-amp.lv2/amp-ui.c)
add_lv2_fixture(eg_enum_lv2 eg-enum.lv2 enumamp
  TTL_FILES enumamp.ttl
  SOURCES eg-enum.lv2/enumamp.c)
add_lv2_fixture(eg_fifths_lv2 eg-fifths.lv2 fifths
  TTL_FILES fifths.ttl
  SOURCES eg-fifths.lv2/fifths.c)
# unmodified upstream plugin (conformance fixture): loads a sample file
# through the worker extension and applies it in run(); samplerate.h in
# the compat/ include dir redirects its libsamplerate usage to soxr's
# libsamplerate-compatible bindings
add_lv2_fixture(eg_sampler_lv2 eg-sampler.lv2 sampler
  TTL_FILES sampler.ttl
  SOURCES eg-sampler.lv2/sampler.c
  DATA_FILES click.wav)
target_include_directories(eg_sampler_lv2 PRIVATE eg-sampler.lv2/compat)
target_link_libraries(eg_sampler_lv2 PRIVATE soxr-lsr SndFile::sndfile)
# stub for the GtkUI the fixture's TTL declares: this host cannot embed
# it; the binary completes the bundle metadata and is never loaded
add_lv2_fixture(eg_sampler_ui_lv2 eg-sampler.lv2 sampler_ui
  NO_MANIFEST
  SOURCES eg-sampler.lv2/sampler_ui.c)
add_lv2_fixture(pos_probe_lv2 pos-probe.lv2 pos_probe
  TTL_FILES pos-probe.ttl
  SOURCES pos-probe.lv2/pos_probe.c)
add_lv2_fixture(state_files_lv2 state-files.lv2 state_files
  TTL_FILES state-files.ttl
  SOURCES state-files.lv2/state_files.c)
add_lv2_fixture(plumbing_lv2 plumbing.lv2 plumbing
  TTL_FILES plumbing.ttl
  SOURCES plumbing.lv2/midieat.c plumbing.lv2/plumbing.c plumbing.lv2/route.c)
add_lv2_fixture(sigabrt_lv2 sigabrt.lv2 sigabrt
  TTL_FILES sigabrt.ttl
  SOURCES sigabrt.lv2/sigabrt.c)
add_lv2_fixture(test_instrument_lv2 test-instrument.lv2 test-instrument
  TTL_FILES test-instrument.ttl
  SOURCES test-instrument.lv2/test-instrument.c)
# preset-only bundle: no plugin library, just a pset:Preset for the
# test-instrument plugin
add_lv2_fixture(test_instrument_preset_lv2 test-instrument.preset.lv2 ""
  TTL_FILES preset.ttl)
# bundle with malformed plugin data (a port without a direction): the host
# must refuse to load it (the library itself is valid so instantiation
# would succeed against the partially read port metadata)
add_lv2_fixture(broken_lv2 broken.lv2 broken
  TTL_FILES broken.ttl
  SOURCES broken.lv2/broken.c)

# Exported so the parent scope can derive the plugin count for test compile
# definitions, and ensure every fixture library (including non-plugin UI
# libraries) builds with the tests
set(zrythm_test_lv2_fixture_targets
  eg_amp_lv2
  eg_amp_ui_lv2
  eg_enum_lv2
  eg_fifths_lv2
  eg_sampler_lv2
  eg_sampler_ui_lv2
  plumbing_lv2
  pos_probe_lv2
  sigabrt_lv2
  state_files_lv2
  test_instrument_lv2
  broken_lv2
  PARENT_SCOPE
)
