# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# Third-party dependency inventory: attribution data (bundled in the app)
# and CycloneDX SBOM (build artifact). Everything is generated from in-repo
# data; no network access. The Conan fragment is produced by conan install
# (see conanfile.py generate()).

set(dependency_manifest "${CMAKE_SOURCE_DIR}/data/dependencies.toml")
set(conan_sbom_fragment "${CMAKE_BINARY_DIR}/generators/zrythm-conan-sbom.cdx.json")
set(generated_attributions_json "${CMAKE_BINARY_DIR}/attributions/attributions.json")
set(generated_sbom_json "${CMAKE_BINARY_DIR}/sbom.cdx.json")

set(attributions_cmd
  ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/generate_attributions.py
  --manifest "${dependency_manifest}"
  --licenses-dirs "${CMAKE_SOURCE_DIR}/LICENSES" "${CMAKE_SOURCE_DIR}/data/licenses"
  --version-file "${CMAKE_SOURCE_DIR}/VERSION.txt"
  --output "${generated_attributions_json}"
)
if(EXISTS "${conan_sbom_fragment}")
  list(APPEND attributions_cmd --conan-fragment "${conan_sbom_fragment}")
endif()
execute_process(
  COMMAND ${attributions_cmd}
  RESULT_VARIABLE attributions_result
)
if(NOT attributions_result EQUAL 0)
  message(FATAL_ERROR "Failed to generate attributions.json")
endif()

set(sbom_cmd
  ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/generate_sbom.py
  --manifest "${dependency_manifest}"
  --licenses-dirs "${CMAKE_SOURCE_DIR}/LICENSES" "${CMAKE_SOURCE_DIR}/data/licenses"
  --version-file "${CMAKE_SOURCE_DIR}/VERSION.txt"
  --output "${generated_sbom_json}"
)
if(EXISTS "${conan_sbom_fragment}")
  list(APPEND sbom_cmd --conan-fragment "${conan_sbom_fragment}")
endif()
execute_process(
  COMMAND ${sbom_cmd}
  RESULT_VARIABLE sbom_result
)
if(NOT sbom_result EQUAL 0)
  # CI regenerates the SBOM strictly; a local configure without the venv
  # python (missing cyclonedx-python-lib) should not fail the build.
  message(WARNING "SBOM generation failed (exit ${sbom_result}); skipping")
endif()

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${dependency_manifest}"
  "${conan_sbom_fragment}"
  "${CMAKE_SOURCE_DIR}/tools/generate_attributions.py"
  "${CMAKE_SOURCE_DIR}/tools/generate_sbom.py"
  "${CMAKE_SOURCE_DIR}/tools/dependency_manifest.py"
)
