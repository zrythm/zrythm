// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

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

/** Why an operation rejected its input or state. */
struct Refusal
{
  std::string reason;
};

enum class OperationStatus
{
  /** The operation completed. */
  Applied,

  /** The operation rejected the request; nothing changed. */
  Refused,

  /** The operation could not run (unknown name, internal failure). */
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

  /** Result payload of a query, or extra data for the caller. */
  nlohmann::json result = nullptr;

  /** Texts of the commands pushed by this invocation, in order. */
  std::vector<std::string> command_texts;

  /** Objects changed by this invocation. */
  std::vector<AffectedObject> affected_objects;

  /**
   * The project revision before and after this invocation, from the undo
   * stack's journal sequence.
   *
   * The two are equal whenever the invocation changed nothing. On a
   * refusal both carry the current revision, so a refused client can
   * re-read and retry.
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
