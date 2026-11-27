# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_mcp_server_lib STATIC)

# The headers are included as <mcpserver/...> through the include directory
# that the file sets export.
target_sources(zrythm_mcp_server_lib
  PRIVATE
    json_rpc.cpp
    mcp_methods.cpp
    mcp_types.cpp
    protocol_engine.cpp
    streamable_http_transport.cpp
    # Headers that only the library itself includes.
    PRIVATE
    FILE_SET private_headers
    TYPE HEADERS
    BASE_DIRS ".."
    FILES
      http_exchange.h
      mcp_methods.h
      streamable_http_transport.h
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ".."
    FILES
      http_request.h
      json_rpc.h
      mcp_server.h
      mcp_types.h
      protocol_engine.h
      resource_provider.h
)

set_target_properties(zrythm_mcp_server_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
  VERIFY_INTERFACE_HEADER_SETS ${ZRYTHM_VERIFY_INTERFACE_HEADER_SETS}
)

target_precompile_headers(zrythm_mcp_server_lib REUSE_FROM zrythm_utils_lib)

# The library depends on Qt and on zrythm_utils_lib only. That library
# brings the compile options, the formatter and the logger.
target_link_libraries(zrythm_mcp_server_lib
  PUBLIC
    zrythm::utils_lib
    nlohmann_json::nlohmann_json
    Qt6::HttpServer
    Qt6::Network
)

add_library(zrythm::mcp_server_lib ALIAS zrythm_mcp_server_lib)
