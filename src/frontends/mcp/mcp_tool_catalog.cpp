// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "frontends/mcp/mcp_tool_catalog.h"

#include <fmt/format.h>

namespace zrythm::frontends::mcp
{

std::expected<McpToolCatalog, std::string>
make_tool_catalog (const operations::OperationRegistry &registry)
{
  McpToolCatalog catalog;
  catalog.tools.reserve (registry.operations ().size ());
  for (const auto &operation : registry.operations ())
    {
      auto tool = zrythm::mcp::Tool::from_json (operation.tool_definition ());
      if (!tool.has_value ())
        {
          return std::unexpected (
            fmt::format (
              "the tool definition of the operation '{}' is not valid: {}",
              operation.name (), tool.error ()));
        }
      catalog.kinds_by_name.emplace (operation.name (), operation.kind ());
      catalog.tools.push_back (std::move (*tool));
    }
  return catalog;
}

} // namespace zrythm::frontends::mcp
