// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "frontends/mcp/mcp_tool_catalog.h"

#include <fmt/format.h>

namespace zrythm::frontends::mcp
{

std::string
mcp_tool_name (const operations::OperationId &id)
{
  return fmt::format ("{}_{}", id.group, id.function);
}

std::expected<McpToolCatalog, std::string>
make_tool_catalog (const operations::OperationRegistry &registry)
{
  McpToolCatalog catalog;
  catalog.tools.reserve (registry.operations ().size ());
  for (const auto &operation : registry.operations ())
    {
      const auto &id = operation.id ();
      const auto  tool_name = mcp_tool_name (id);
      if (tool_name.size () > kMaxToolNameLength)
        {
          return std::unexpected (
            fmt::format (
              "the MCP tool name '{}' of the operation '{}' is longer than "
              "{} characters",
              tool_name, id.to_string (), kMaxToolNameLength));
        }
      auto tool =
        zrythm::mcp::Tool::from_json (operation.tool_definition (tool_name));
      if (!tool.has_value ())
        {
          return std::unexpected (
            fmt::format (
              "the tool definition of the operation '{}' is not valid: {}",
              id.to_string (), tool.error ()));
        }
      catalog.by_name.emplace (
        tool_name, McpCatalogTool{ id, operation.kind () });
      catalog.tools.push_back (std::move (*tool));
    }
  return catalog;
}

} // namespace zrythm::frontends::mcp
