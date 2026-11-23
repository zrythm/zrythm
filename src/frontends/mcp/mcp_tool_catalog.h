// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QtMcpCommon/QMcpTool>

#include "operations/operation.h"
#include "operations/operation_registry.h"

namespace zrythm::frontends::mcp
{

/**
 * The tools an MCP connection can call, snapshotted from the operation
 * registry.
 *
 * The catalog is built once per endpoint start and never mutated, so the
 * worker thread can read it without synchronization. Tools map one-to-one
 * to operations; their schemas are the registry's own tool definitions.
 */
struct McpToolCatalog
{
  /** One tool per operation, in registration order. */
  QList<QMcpTool> tools;

  /** The operation kind of each tool, by operation name. */
  QHash<QString, operations::OperationKind> kinds_by_name;
};

/** Snapshots the registry's operations as the tool catalog. */
McpToolCatalog
make_tool_catalog (const operations::OperationRegistry &registry);

} // namespace zrythm::frontends::mcp
