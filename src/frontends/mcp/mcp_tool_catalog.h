// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstddef>
#include <expected>
#include <string>
#include <unordered_map>
#include <vector>

#include "operations/operation.h"
#include "operations/operation_registry.h"
#include <mcpserver/mcp_types.h>

namespace zrythm::frontends::mcp
{

/**
 * The longest tool name the catalog publishes.
 *
 * The narrowest name limit of the function-calling interfaces of model
 * providers is 64 characters.
 */
inline constexpr std::size_t kMaxToolNameLength = 64;

/**
 * The MCP tool name of @p id: its group, an underscore, and its
 * function.
 */
std::string
mcp_tool_name (const operations::OperationId &id);

/** The operation a catalog tool calls. */
struct McpCatalogTool
{
  operations::OperationId   id;
  operations::OperationKind kind;
};

/**
 * The tools an MCP connection can call, snapshotted from the operation
 * registry.
 *
 * The catalog is built once per endpoint start and never mutated, so the
 * worker thread can read it without synchronization. Tools map one-to-one
 * to operations. A tool is named by mcp_tool_name(), and its definition
 * is the one the registry builds for that name.
 */
struct McpToolCatalog
{
  /** One tool per operation, in registration order. */
  std::vector<zrythm::mcp::Tool> tools;

  /** The operation of each tool, by tool name. */
  std::unordered_map<std::string, McpCatalogTool> by_name;
};

/**
 * @brief Snapshots the operations of @p registry as the tool catalog.
 *
 * Returns an error that names the operation when its tool name is longer
 * than kMaxToolNameLength or its tool definition is not valid for the
 * MCP server.
 */
std::expected<McpToolCatalog, std::string>
make_tool_catalog (const operations::OperationRegistry &registry);

} // namespace zrythm::frontends::mcp
