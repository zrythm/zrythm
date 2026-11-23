// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "frontends/mcp/mcp_json.h"
#include "frontends/mcp/mcp_tool_catalog.h"
#include "utils/utf8_string.h"

namespace zrythm::frontends::mcp
{

McpToolCatalog
make_tool_catalog (const operations::OperationRegistry &registry)
{
  McpToolCatalog catalog;
  catalog.tools.reserve (
    static_cast<qsizetype> (registry.operations ().size ()));
  for (const auto &operation : registry.operations ())
    {
      QMcpTool tool;
      tool.fromJsonObject (
        qjson_from_nlohmann (operation.tool_definition ()).toObject ());
      catalog.tools.append (std::move (tool));
      catalog.kinds_by_name.insert (
        utils::Utf8String::from_utf8_encoded_string (operation.name ())
          .to_qstring (),
        operation.kind ());
    }
  return catalog;
}

} // namespace zrythm::frontends::mcp
