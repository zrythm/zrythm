// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <optional>
#include <ranges>
#include <span>
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
        parameter_names_ (parameter_names_of (op)),
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

  /** The names of the operation's parameters, in declaration order. */
  std::span<const std::string_view> parameter_names () const
  {
    return parameter_names_;
  }

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
  /** The names of the parameters of @p op, in declaration order. */
  template <typename Handler, std::size_t NumParams>
  static std::vector<std::string_view>
  parameter_names_of (const TypedOperation<Handler, NumParams> &op)
  {
    return op.parameter_specs ()
           | std::views::transform ([] (const ParamSpec &spec) {
               return spec.name;
             })
           | std::ranges::to<std::vector> ();
  }

  OperationKind                                    kind_;
  OperationId                                      id_;
  std::string                                      summary_;
  std::vector<std::string_view>                    parameter_names_;
  std::function<nlohmann::json (std::string_view)> make_tool_definition_;
  std::function<OperationReport (const nlohmann::json &, OperationEnvironment &)>
    invoke_;
};

/**
 * Options of a dispatch, checked by the registry before the operation
 * runs.
 */
struct DispatchOptions
{
  /**
   * The project revision the caller last saw.
   *
   * An Edit is refused when this is not set or does not match the undo
   * stack's current revision. Query and Control operations ignore it.
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
   * breaks a naming rule, is already registered, or one of its parameter
   * names breaks a naming rule.
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
    if (
      const auto broken_rule = check_parameter_names (op.parameter_names ());
      broken_rule.has_value ())
      {
        z_warning (
          "Ignoring operation '{}': {}", op.id ().to_string (), *broken_rule);
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
   * Refused report. An Edit is refused with MissingRevision when it carries
   * no expected revision, and with StaleRevision when the revision is not
   * the current one. Both checks run before argument validation. Dispatch
   * is not synchronized: every call must happen on one thread.
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
        return unchanged_report (
          OperationStatus::Error,
          fmt::format ("unknown operation '{}'", id.to_string ()),
          current_revision);
      }
    if (
      op->kind () == OperationKind::Edit
      && !options.expected_revision.has_value ())
      {
        return refused_report (
          utils::RefusalKind::MissingRevision,
          fmt::format (
            "expected_revision is required; the current revision is {}. "
            "Re-read the state, then retry with expected_revision set to "
            "it.",
            current_revision),
          current_revision);
      }
    if (
      options.expected_revision.has_value () && op->kind () == OperationKind::Edit
      && *options.expected_revision != current_revision)
      {
        return refused_report (
          utils::RefusalKind::StaleRevision,
          fmt::format (
            "expected revision {} but the current revision is {}; re-read and "
            "retry",
            *options.expected_revision, current_revision),
          current_revision);
      }
    return op->invoke (args, env);
  }

private:
  /**
   * A report of a request that changed nothing: @p status, @p message,
   * and @p revision before and after.
   */
  static OperationReport unchanged_report (
    OperationStatus status,
    std::string     message,
    std::uint64_t   revision)
  {
    OperationReport report;
    report.status = status;
    report.message = std::move (message);
    report.revision_before = revision;
    report.revision_after = revision;
    return report;
  }

  /** A refused report that carries @p kind and changed nothing. */
  static OperationReport refused_report (
    utils::RefusalKind kind,
    std::string        message,
    std::uint64_t      revision)
  {
    auto report = unchanged_report (
      OperationStatus::Refused, std::move (message), revision);
    report.refusal_kind = kind;
    return report;
  }

  std::vector<Operation> operations_;
};

} // namespace zrythm::operations
