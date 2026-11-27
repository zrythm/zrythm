// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>

#include <QString>

#include "operations/operation.h"

namespace zrythm::frontends::mcp
{

/**
 * What a connection may do.
 *
 * Each token carries one grant; a request that authenticates with that
 * token is held to it.
 */
enum class McpGrant
{
  /** Query operations only. */
  Query,
  /** Query and Control operations. */
  Control,
  /** Query, Control and Edit operations. */
  Full,
};

/** The wire name of the grant, as a token entry spells it. */
inline QString
grant_identity (McpGrant grant)
{
  switch (grant)
    {
    case McpGrant::Query:
      return QStringLiteral ("query");
    case McpGrant::Control:
      return QStringLiteral ("control");
    case McpGrant::Full:
      return QStringLiteral ("full");
    }
  return QStringLiteral ("query");
}

/** The grant for a wire name, or no value when the name is unknown. */
inline std::optional<McpGrant>
grant_from_identity (const QString &identity)
{
  if (identity == QStringLiteral ("query"))
    return McpGrant::Query;
  if (identity == QStringLiteral ("control"))
    return McpGrant::Control;
  if (identity == QStringLiteral ("full"))
    return McpGrant::Full;
  return std::nullopt;
}

/** Whether the grant allows an operation of the given kind. */
constexpr bool
grant_allows (McpGrant grant, operations::OperationKind kind)
{
  switch (grant)
    {
    case McpGrant::Query:
      return kind == operations::OperationKind::Query;
    case McpGrant::Control:
      return kind != operations::OperationKind::Edit;
    case McpGrant::Full:
      return true;
    }
  return false;
}

} // namespace zrythm::frontends::mcp
