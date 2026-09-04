# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# LV2 test fixtures: real C plugins written against the LV2 spec headers.
# Each fixture builds a self-contained bundle under
# <ZRYTHM_LV2_FIXTURE_ROOT>/$<CONFIG>/<name>.lv2/ (manifest, ttl and the
# plugin binary side by side, as a real .lv2 bundle is laid out).

# add_lv2_fixture(<target> <bundle-dir-name> <binary-name>
#   TTL_FILES <ttl...> SOURCES <sources...>)
#
# SOURCES may be omitted for ttl-only bundles (e.g. presets), which contain no
# plugin library at all.
function(add_lv2_fixture target bundle_dir output_name)
  cmake_parse_arguments(ARG "" "" "TTL_FILES;SOURCES" ${ARGN})
  set(bundle_root ${ZRYTHM_LV2_FIXTURE_ROOT}/$<CONFIG>/${bundle_dir})

  # The bundle path contains $<CONFIG>, which configure_file() cannot emit,
  # so files are staged with configure_file() and then generated into the
  # bundle with file(GENERATE)
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${bundle_dir}/manifest.ttl.in)
    set(manifest_src ${CMAKE_CURRENT_SOURCE_DIR}/${bundle_dir}/manifest.ttl.in)
  else()
    set(manifest_src ${CMAKE_CURRENT_SOURCE_DIR}/${bundle_dir}/manifest.ttl)
  endif()
  configure_file(${manifest_src} ${bundle_dir}.manifest.ttl.staged @ONLY)
  file(READ ${CMAKE_CURRENT_BINARY_DIR}/${bundle_dir}.manifest.ttl.staged manifest_content)
  file(GENERATE OUTPUT ${bundle_root}/manifest.ttl CONTENT "${manifest_content}")

  foreach(ttl_file ${ARG_TTL_FILES})
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/${bundle_dir}/${ttl_file} ttl_content)
    file(GENERATE OUTPUT ${bundle_root}/${ttl_file} CONTENT "${ttl_content}")
  endforeach()

  if(NOT ARG_SOURCES)
    return()
  endif()

  add_library(${target} MODULE ${ARG_SOURCES})
  # Sanitizers (when enabled) let violations inside the fixture be caught;
  # the fixtures are only ever loaded by sanitizer-enabled test binaries
  target_link_libraries(${target} PRIVATE lv2::lv2 zrythm::sanitizers)
  set_target_properties(${target} PROPERTIES
    PREFIX ""
    OUTPUT_NAME ${output_name}
    LIBRARY_OUTPUT_DIRECTORY ${bundle_root}
    UNITY_BUILD OFF
    AUTOMOC OFF
  )
  zrythm_target_disable_warnings(${target})
endfunction()

add_lv2_fixture(eg_amp_lv2 eg-amp.lv2 amp
  TTL_FILES amp.ttl
  SOURCES eg-amp.lv2/amp.c)
add_lv2_fixture(eg_fifths_lv2 eg-fifths.lv2 fifths
  TTL_FILES fifths.ttl
  SOURCES eg-fifths.lv2/fifths.c)
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

# Exported so the parent scope can derive the plugin count for test compile
# definitions
set(zrythm_test_lv2_fixture_targets
  eg_amp_lv2
  eg_fifths_lv2
  plumbing_lv2
  sigabrt_lv2
  test_instrument_lv2
  PARENT_SCOPE
)
