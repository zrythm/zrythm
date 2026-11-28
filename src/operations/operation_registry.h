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
      : kind_ (op.kind ()), id_ (op.id ()), summary_ (op.summary ()),
        make_tool_definition_ ([op] (std::string_view tool_name) -> nlohmann::json {
          return op.make_tool_definition (tool_name);
        }),
        invoke_ (
          [op] (const nlohmann::json &args, OperationEnvironment &env)
            -> OperationReport { return op.invoke (args, env); })
  {
  }

  OperationKind      kind () const { return kind_; }
  const OperationId &id () const { return id_; }
  const std::string &summary () const { return summary_; }

  /** The tool definition of the operation, published under @p tool_name. */
  nlohmann::json tool_definition (std::string_view tool_name) const
  {
    return make_tool_definition_ (tool_name);
  }

  OperationReport
  invoke (const nlohmann::json &args, OperationEnvironment &env) const
  {
    return invoke_ (args, env);
  }

private:
  OperationKind                                    kind_;
  OperationId                                      id_;
  std::string                                      summary_;
  std::function<nlohmann::json (std::string_view)> make_tool_definition_;
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
 * The operations available in a session, found by id.
 */
class OperationRegistry
{
public:
  /**
   * Registers an operation, or ignores it with a warning when its id
   * breaks a naming rule or is already registered.
   */
  void register_operation (Operation op)
  {
    if (
      const auto broken_rule = check_operation_id (op.id ());
      broken_rule.has_value ())
      {
        z_warning (
          "Ignoring operation '{}': {}", op.id ().to_string (), *broken_rule);
        return;
      }
    if (find (op.id ()) != nullptr)
      {
        z_warning (
          "Ignoring operation '{}': this id is already registered",
          op.id ().to_string ());
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

  /** Returns the operation with the given id, or null. */
  const Operation * find (OperationId id) const
  {
    const auto it = std::ranges::find_if (
      operations_, [id] (const Operation &op) { return op.id () == id; });
    return it != operations_.end () ? &*it : nullptr;
  }

  /** All registered operations, in registration order. */
  const std::vector<Operation> &operations () const { return operations_; }

  /**
   * Looks up the operation by id and runs it.
   *
   * An unknown id is an Error report; anything the operation rejects is a
   * Refused report. The expected-revision check runs before argument
   * validation. Dispatch is not synchronized: every call must happen on
   * one thread.
   */
  OperationReport dispatch (
    OperationId            id,
    const nlohmann::json  &args,
    OperationEnvironment  &env,
    const DispatchOptions &options = {}) const
  {
    const auto   current_revision = env.undo_stack ().revision ();
    const auto * op = find (id);
    if (op == nullptr)
      {
        OperationReport report;
        report.status = OperationStatus::Error;
        report.message = fmt::format ("unknown operation '{}'", id.to_string ());
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
        report.refusal_kind = utils::RefusalKind::StaleRevision;
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
