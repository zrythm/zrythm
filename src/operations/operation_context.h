// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "undo/undo_stack.h"
#include "utils/utf8_string.h"

#include <QUuid>

#include "operations/operation.h"
#include "operations/operation_environment.h"
#include <fmt/format.h>

namespace zrythm::operations
{

/**
 * Per-invocation access to the environment for one operation handler.
 *
 * Handlers never touch the undo stack or the models directly: they push
 * commands and record affected objects through the context, and build
 * their report from it. The dispatcher merges what the context collected
 * into the report the handler returns.
 */
class OperationContext
{
public:
  explicit OperationContext (OperationEnvironment &env) : env_ (&env) { }

  OperationEnvironment &environment () const { return *env_; }

  /**
   * Pushes an undo command (taking ownership) and records its text for
   * the report.
   *
   * The text is recorded only when the stack accepts the command. A
   * refused push is counted instead; a report with refused pushes
   * becomes an error report.
   */
  void push_command (std::unique_ptr<commands::Command> cmd)
  {
    const auto text =
      std::string (utils::Utf8String::from_qstring (cmd->text ()).view ());
    if (env_->undo_stack ().push (std::move (cmd)))
      {
        pending_command_texts_.push_back (std::move (text));
      }
    else
      {
        ++failed_push_count_;
      }
  }

  /** Records an object changed by this invocation. */
  void note_affected (std::string uuid, std::string kind)
  {
    pending_affected_.push_back (
      AffectedObject{ std::move (uuid), std::move (kind) });
  }

  /** Builds an applied report with an optional result payload. */
  [[nodiscard]] OperationReport applied (nlohmann::json result = nullptr) const
  {
    OperationReport report;
    report.status = OperationStatus::Applied;
    report.result = std::move (result);
    return report;
  }

  /** Builds a refused report from an operation-local reason string. */
  [[nodiscard]] OperationReport
  refused (utils::RefusalKind kind, std::string reason) const
  {
    OperationReport report;
    report.status = OperationStatus::Refused;
    report.refusal_kind = kind;
    report.message = std::move (reason);
    return report;
  }

  /** Builds a refused report from a domain refusal reason. */
  [[nodiscard]] OperationReport refused (utils::RefusalReason reason) const
  {
    OperationReport report;
    report.status = OperationStatus::Refused;
    report.refusal_kind = reason.kind;
    report.message = reason.text.str ();
    return report;
  }

  /**
   * Returns why @p uuid may not be the target of an Edit, or no value
   * when it may.
   *
   * The undo history is project-scoped: an object that resolves through
   * the session but not through the project registry is application
   * state, and an object that resolves through neither is not
   * registered. @p alternative names the Control operation to suggest
   * for application state, when one exists.
   */
  [[nodiscard]] std::optional<utils::RefusalReason> edit_target_refusal (
    const QUuid             &id,
    const utils::Utf8String &label,
    std::string_view         alternative = {}) const
  {
    const auto &env = environment ();
    if (env.project_object_registry ().contains (id))
      {
        return std::nullopt;
      }
    auto name = label;
    if (name.empty ())
      {
        name =
          utils::Utf8String::from_qstring (id.toString (QUuid::WithoutBraces));
      }
    std::string hint;
    if (!alternative.empty ())
      {
        hint = fmt::format (", use {}", alternative);
      }
    if (env.object_registry ().contains (id))
      {
        return utils::RefusalReason{
          utils::RefusalKind::Unsupported,
          utils::Utf8String::from_utf8_encoded_string (
            fmt::format (
              "'{}' is application state: edits are project-scoped{}",
              name.view (), hint))
        };
      }
    return utils::RefusalReason{
      utils::RefusalKind::NotFound,
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format (
          "'{}' is not registered in this session's registry", name.view ()))
    };
  }

  /** Number of commands the stack refused during this invocation. */
  [[nodiscard]] int failed_push_count () const { return failed_push_count_; }

  std::vector<std::string> take_command_texts ()
  {
    return std::move (pending_command_texts_);
  }

  std::vector<AffectedObject> take_affected_objects ()
  {
    return std::move (pending_affected_);
  }

private:
  OperationEnvironment *      env_;
  std::vector<std::string>    pending_command_texts_;
  std::vector<AffectedObject> pending_affected_;
  int                         failed_push_count_ = 0;
};

} // namespace zrythm::operations
