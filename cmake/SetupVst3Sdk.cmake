# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# Sets up the VST3 SDK fetched via CPM (expects vst3sdk_SOURCE_DIR).
#
# Only creates the library targets we need (base, pluginterfaces, sdk_common,
# sdk, sdk_hosting) - no examples, VSTGUI or other tooling - using the SDK's
# own cmake modules. Consumers:
#   - plugins: smtg_add_vst3plugin(<tgt> ...) + target_link_libraries(<tgt> sdk)
#   - hosts: target_link_libraries(<tgt> sdk_hosting)
#
# The include() is wrapped in a function because SMTG_Global.cmake applies
# unwanted directory-scope settings at include time (output directories,
# etc.). Platform variables (SMTG_LINUX & friends) are CACHE INTERNAL and
# remain available globally, and all smtg_* functions are global once defined.

# Options consumed by smtg_* functions - set before the include so the SMTG
# option() declarations pick them up (CMP0077)
set(SMTG_CREATE_PLUGIN_LINK OFF)
set(SMTG_RUN_VST_VALIDATOR OFF)
set(SMTG_CREATE_MODULE_INFO OFF)

function (zrythm_setup_vst3sdk_targets)
  list(APPEND CMAKE_MODULE_PATH "${vst3sdk_SOURCE_DIR}/cmake/modules")
  include(SMTG_VST3_SDK)
  set(SDK_ROOT "${vst3sdk_SOURCE_DIR}")
  set(public_sdk_SOURCE_DIR "${SDK_ROOT}/public.sdk")
  set(pluginterfaces_SOURCE_DIR "${SDK_ROOT}/pluginterfaces")
  smtg_create_lib_base_target()
  smtg_create_pluginterfaces_target()
  smtg_create_public_sdk_common_target()
  smtg_create_public_sdk_target()
  smtg_create_public_sdk_hosting_target()
endfunction ()
zrythm_setup_vst3sdk_targets()

# Consumed by smtg_target_add_library_main() (via smtg_add_vst3plugin()) and
# by the validator's CMakeLists.txt - set at directory scope so subdirectories
# can see them
set(SDK_ROOT "${vst3sdk_SOURCE_DIR}")
set(public_sdk_SOURCE_DIR "${SDK_ROOT}/public.sdk")
set(pluginterfaces_SOURCE_DIR "${SDK_ROOT}/pluginterfaces")

foreach (vst3sdk_tgt base pluginterfaces sdk_common sdk sdk_hosting)
  zrythm_target_disable_warnings(${vst3sdk_tgt})
endforeach ()
target_link_libraries(base PUBLIC Threads::Threads ${CMAKE_DL_LIBS})

# Treat SDK headers as system headers in consumers so our strict warning flags
# don't fire on them (the SDK headers trip -Wundef, -Wnon-virtual-dtor, etc.)
target_include_directories(sdk_hosting SYSTEM INTERFACE ${vst3sdk_SOURCE_DIR})

if (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  # threadchecker_{linux,mac}.{cpp,mm} use std::terminate() without including
  # <exception> (fails with libc++; not fixed upstream as of v3.8.0_build_66)
  target_compile_options(sdk_common PRIVATE -include exception)
endif ()

# Work around the VST3 SDK's CMake assuming a multi-config generator on
# Windows. smtg_target_make_plugin_package() only sets the inner-bundle
# LIBRARY_OUTPUT_DIRECTORY via a foreach(CMAKE_CONFIGURATION_TYPES) loop
# (SMTG_AddSMTGLibrary.cmake:378). With single-config generators (Ninja),
# that loop is empty, so the DLL output path collides with the PRE_BUILD
# step that creates the bundle directory — the linker gets LNK1104 because
# it is asked to create a file where a directory already exists.
#
# This function mirrors the SDK's multi-config logic: it redirects the DLL
# inside the bundle at Contents/<arch>/ and ensures that directory exists
# (link.exe, unlike mold/lld, does not create parent directories).
#
# No-op on non-Windows, multi-config generators, or when bundle creation
# is disabled.
function (zrythm_fix_vst3_ninja_bundle_output_dir target)
  get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(NOT WIN32 OR is_multi_config OR NOT SMTG_CREATE_BUNDLE_FOR_WINDOWS)
    return()
  endif()

  get_target_property(pkg_name   ${target} SMTG_PLUGIN_PACKAGE_NAME)
  get_target_property(binary_dir ${target} SMTG_PLUGIN_BINARY_DIR)
  get_target_property(contents   ${target} SMTG_PLUGIN_PACKAGE_CONTENTS)
  get_target_property(arch       ${target} SMTG_WIN_ARCHITECTURE_NAME)

  if(NOT pkg_name OR NOT binary_dir OR NOT contents OR NOT arch)
    message(FATAL_ERROR
      "zrythm_fix_vst3_ninja_bundle_output_dir(${target}): "
      "missing SDK target properties. Call this after smtg_add_vst3plugin().")
  endif()

  set(output_dir "${binary_dir}/${pkg_name}/${contents}/${arch}")
  set_target_properties(${target} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${output_dir}")
  add_custom_command(TARGET ${target} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}")
endfunction ()
