// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "utils/logger.h"

#include "operations/operation.h"
#include "operations/operation_environment.h"
#include "operations/typed_operation.h"
#include <fmt/format.h>

namespace zrythm::operations
{

/**
 * A type-erased operation: the dispatchable form of a TypedOperation.
 */
class Operation
{
public:
  template <typename Handler, std::size_t NumParams>
  Operation (const TypedOperation<Handler, NumParams> &op)
      : kind_ (op.kind ()), name_ (op.name ()), summary_ (op.summary ()),
        tool_definition_ (op.make_tool_definition ()),
        invoke_ (
          [op] (const nlohmann::json &args, OperationEnvironment &env)
            -> OperationReport { return op.invoke (args, env); })
  {
  }

  OperationKind         kind () const { return kind_; }
  const std::string    &name () const { return name_; }
  const std::string    &summary () const { return summary_; }
  const nlohmann::json &tool_definition () const { return tool_definition_; }

  OperationReport
  invoke (const nlohmann::json &args, OperationEnvironment &env) const
  {
    return invoke_ (args, env);
  }

private:
  OperationKind  kind_;
  std::string    name_;
  std::string    summary_;
  nlohmann::json tool_definition_;
  std::function<OperationReport (const nlohmann::json &, OperationEnvironment &)>
    invoke_;
};

/**
 * Optional dispatch policies, applied by the registry before the operation
 * runs.
 */
struct DispatchOptions
{
  /**
   * The project revision the caller last saw.
   *
   * Edit operations are refused when this does not match the undo stack's
   * current revision, so a client working from stale state cannot
   * overwrite another client's changes unnoticed. Query and Control
   * operations ignore it.
   */
  std::optional<std::uint64_t> expected_revision;
};

/**
 * The set of operations available in a session, by name.
 */
class OperationRegistry
{
public:
  /**
   * Registers an operation, or ignores it with a warning when its name is
   * already taken.
   */
  void register_operation (Operation op)
  {
    if (find (op.name ()) != nullptr)
      {
        z_warning (
          "Ignoring operation '{}': this name is already registered",
          op.name ());
        return;
      }
    operations_.push_back (std::move (op));
  }

  void register_operations (std::vector<Operation> ops)
  {
    for (auto &op : ops)
      {
        register_operation (std::move (op));
      }
  }

  /** Returns the operation with the given name, or null. */
  const Operation * find (std::string_view name) const
  {
    const auto it = std::ranges::find_if (
      operations_, [name] (const Operation &op) { return op.name () == name; });
    return it != operations_.end () ? &*it : nullptr;
  }

  /** All registered operations, in registration order. */
  const std::vector<Operation> &operations () const { return operations_; }

  /** The MCP-style tool definitions of all operations. */
  std::vector<nlohmann::json> tool_definitions () const
  {
    auto defs = std::vector<nlohmann::json>{};
    defs.reserve (operations_.size ());
    for (const auto &op : operations_)
      {
        defs.push_back (op.tool_definition ());
      }
    return defs;
  }

  /**
   * Looks up the operation by name and runs it.
   *
   * An unknown name is an Error report; anything the operation rejects is a
   * Refused report. The expected-revision check runs before argument
   * validation. Dispatch is not synchronized: every call must happen on
   * one thread.
   */
  OperationReport dispatch (
    std::string_view       name,
    const nlohmann::json  &args,
    OperationEnvironment  &env,
    const DispatchOptions &options = {}) const
  {
    const auto   current_revision = env.undo_stack ().revision ();
    const auto * op = find (name);
    if (op == nullptr)
      {
        OperationReport report;
        report.status = OperationStatus::Error;
        report.message = fmt::format ("unknown operation '{}'", name);
        report.revision_before = current_revision;
        report.revision_after = current_revision;
        return report;
      }
    if (
      options.expected_revision.has_value () && op->kind () == OperationKind::Edit
      && *options.expected_revision != current_revision)
      {
        OperationReport report;
        report.status = OperationStatus::Refused;
        report.message = fmt::format (
          "expected revision {} but the current revision is {}; re-read and "
          "retry",
          *options.expected_revision, current_revision);
        report.revision_before = current_revision;
        report.revision_after = current_revision;
        return report;
      }
    return op->invoke (args, env);
  }

private:
  std::vector<Operation> operations_;
};

} // namespace zrythm::operations
