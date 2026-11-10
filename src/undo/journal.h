// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace zrythm::undo
{

/**
 * @brief The kind of history mutation a journal entry records.
 */
enum class JournalEntryKind : std::uint8_t
{
  /** A command was executed: appended or merged into the history. */
  Push,
  /** The most recently done command was reverted. */
  Undo,
  /** A previously undone command was applied again. */
  Redo,
  /** Commands were reverted and dropped: a transaction abort or a
   * discard_from(). */
  Abort,
};

/**
 * @brief One history mutation, as plain data.
 *
 * The undo stack emits one entry per mutation through its journal
 * callback. Entries carry no Qt or project types, so any observer can
 * consume them without pulling in GUI headers.
 */
struct JournalEntry
{
  /**
   * The stack revision after the mutation. Journal entries number
   * revisions 1..N in emission order; a revision number is never
   * reused.
   */
  std::uint64_t revision = 0;

  JournalEntryKind kind = JournalEntryKind::Push;

  /** Texts of the commands the mutation executed, reverted or dropped. */
  std::vector<std::string> command_texts;
};

} // namespace zrythm::undo
