// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "undo/undo_stack.h"
#include "utils/utf8_string.h"

#include "operations/operation.h"
#include "operations/operation_environment.h"

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
   */
  void push_command (std::unique_ptr<commands::Command> cmd)
  {
    pending_command_texts_.push_back (
      std::string (utils::Utf8String::from_qstring (cmd->text ()).view ()));
    env_->undo_stack ().push (std::move (cmd));
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

  /** Builds a refused report; the project is unchanged. */
  [[nodiscard]] OperationReport refused (std::string reason) const
  {
    OperationReport report;
    report.status = OperationStatus::Refused;
    report.message = std::move (reason);
    return report;
  }

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
};

} // namespace zrythm::operations
