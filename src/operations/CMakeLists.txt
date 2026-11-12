# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_operations_lib STATIC)

target_sources(zrythm_operations_lib
  PRIVATE
    builtin_operations.cpp
    operation_environment.cpp
    track_operations.cpp
    transport_operations.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ".."
    FILES
      builtin_operations.h
      operation.h
      operation_context.h
      operation_environment.h
      operation_registry.h
      param_types.h
      track_operations.h
      transport_operations.h
      typed_operation.h
)

set_target_properties(zrythm_operations_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
  VERIFY_INTERFACE_HEADER_SETS ${ZRYTHM_VERIFY_INTERFACE_HEADER_SETS}
)

target_precompile_headers(zrythm_operations_lib REUSE_FROM zrythm_undo_lib)

target_link_libraries(zrythm_operations_lib
  PUBLIC
    ${zrythm_link_libs}
    zrythm_commands_lib
    zrythm_dsp_lib
    zrythm_structure_tracks_lib
    zrythm_undo_lib
    zrythm::include_dirs
    zrythm::all_compile_options
)
