// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "utils/refusal.h"

#include <nlohmann/json.hpp>

namespace zrythm::operations
{

/**
 * What part of the world an operation touches.
 *
 * - Edit: changes the project document. Always goes through undo commands.
 * - Control: changes runtime application state (engine, transport, files).
 * - Query: returns data without changing anything.
 */
enum class OperationKind
{
  Edit,
  Control,
  Query,
};

/**
 * The identity of an operation: the group it belongs to and its function
 * inside that group.
 *
 * The group is one lowercase word. The function is lowercase snake_case:
 * a lowercase letter, then lowercase letters, digits and underscores. A
 * frontend renders the two parts in the syntax of its protocol. Both
 * strings must have static storage duration, like string literals.
 */
struct OperationId
{
  std::string_view group;
  std::string_view function;

  bool operator== (const OperationId &) const = default;

  /** The id in its written form, "group.function". */
  [[nodiscard]] std::string to_string () const
  {
    return std::string (group) + "." + std::string (function);
  }
};

/** The naming rule @p id breaks, or no value when the id is valid. */
[[nodiscard]] inline std::optional<std::string_view>
check_operation_id (OperationId id)
{
  const auto is_letter = [] (char c) { return c >= 'a' && c <= 'z'; };
  const auto is_digit = [] (char c) { return c >= '0' && c <= '9'; };
  if (id.group.empty () || !is_letter (id.group.front ()))
    {
      return "the group must start with a lowercase letter";
    }
  if (!std::ranges::all_of (id.group, [&] (char c) {
        return is_letter (c) || is_digit (c);
      }))
    {
      return "the group must be one lowercase word";
    }
  if (id.function.empty () || !is_letter (id.function.front ()))
    {
      return "the function must start with a lowercase letter";
    }
  if (!std::ranges::all_of (id.function, [&] (char c) {
        return is_letter (c) || is_digit (c) || c == '_';
      }))
    {
      return "the function must be lowercase snake_case";
    }
  return std::nullopt;
}

/** Why an operation rejected its input or state. */
struct Refusal
{
  utils::RefusalKind kind;
  std::string        reason;
};

enum class OperationStatus
{
  /** The operation completed. */
  Applied,

  /** The operation rejected the request; nothing changed. */
  Refused,

  /**
   * The operation could not run, or ran only partially (unknown name, a
   * refused command push, internal failure). command_texts lists what
   * ran; the caller must not assume a retry is safe.
   */
  Error,
};

/** An object the operation changed, identified by registry UUID. */
struct AffectedObject
{
  std::string uuid;
  std::string kind;
};

/**
 * The outcome of one operation invocation.
 *
 * A refusal is a normal outcome: the operation validated its input or the
 * project state and rejected the request with a reason, and the project is
 * unchanged.
 */
struct OperationReport
{
  OperationStatus status = OperationStatus::Error;

  /** Refusal reason or error message; empty when applied. */
  std::string message;

  /** Why the operation refused; meaningful when status is Refused. */
  utils::RefusalKind refusal_kind = utils::RefusalKind::InvalidParameter;

  /** Result payload of a query, or extra data for the caller. */
  nlohmann::json result = nullptr;

  /** Texts of the commands the stack accepted, in order. */
  std::vector<std::string> command_texts;

  /** Objects changed by this invocation. */
  std::vector<AffectedObject> affected_objects;

  /**
   * The project revision before and after this invocation, from the undo
   * stack's journal sequence.
   *
   * The two are equal whenever the invocation pushed no history entry;
   * Control operations push none, so their applied reports can describe
   * changes the revisions do not track. On a refusal both carry the
   * current revision, so a refused client can re-read and retry.
   */
  std::uint64_t revision_before = 0;
  std::uint64_t revision_after = 0;

  bool applied () const { return status == OperationStatus::Applied; }
  bool refused () const { return status == OperationStatus::Refused; }
  bool failed () const { return status == OperationStatus::Error; }
};

/**
 * Name and documentation of one operation parameter.
 *
 * Both strings must have static storage duration, like string literals.
 */
struct ParamSpec
{
  std::string_view name;
  std::string_view description;
};

} // namespace zrythm::operations
