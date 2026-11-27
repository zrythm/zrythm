# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_subdirectory(mcpserver)

qt_add_library(zrythm_frontends_mcp_lib STATIC)

target_sources(zrythm_frontends_mcp_lib
  PRIVATE
    mcp_endpoint.cpp
    mcp_service.cpp
    mcp_tool_catalog.cpp
    mcp_worker.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ".."
    FILES
      mcp_endpoint.h
      mcp_grant.h
      mcp_service.h
      mcp_tool_catalog.h
      mcp_worker.h
)

set_target_properties(zrythm_frontends_mcp_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
  VERIFY_INTERFACE_HEADER_SETS ${ZRYTHM_VERIFY_INTERFACE_HEADER_SETS}
)

target_precompile_headers(zrythm_frontends_mcp_lib REUSE_FROM zrythm_operations_lib)

target_link_libraries(zrythm_frontends_mcp_lib
  PUBLIC
    zrythm_operations_lib
    zrythm_undo_lib
    zrythm::include_dirs
    zrythm::all_compile_options
    zrythm_mcp_server_lib
)

qt_add_qml_module(zrythm_frontends_mcp_lib
  URI ZrythmMcp
  VERSION 1.0
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmMcp
)
set_target_properties(zrythm_frontends_mcp_libplugin PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)
