// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <compare>
#include <cstdint>
#include <utility>

#include <QUndoCommand>

namespace zrythm::commands
{

/**
 * @brief Base class for all undoable commands.
 *
 * Every command states at construction what the audio engine must do around
 * its undo() and redo() when the undo stack runs it. The stack aggregates
 * these declarations across a command and its macro children and cooperates
 * with the engine accordingly.
 */
class Command : public QUndoCommand
{
public:
  /**
   * @brief Cooperation the audio engine needs when this command runs.
   *
   * The values are ordered by strength: RecalculateGraph implies
   * PauseProcessing.
   */
  enum class EngineRequirement : std::uint8_t
  {
    /** The engine can keep processing while this command runs. */
    None,
    /** The engine must pause processing: the change touches data the audio
     * thread reads (e.g. the tempo map) and must not race with it. */
    PauseProcessing,
    /** Routing changed: the engine must pause processing and rebuild the
     * processing graph before resuming. */
    RecalculateGraph,
  };

  [[nodiscard]] EngineRequirement engine_requirement () const
  {
    return engine_requirement_;
  }

protected:
  Command (const QString &text, EngineRequirement engine_requirement)
      : QUndoCommand (text), engine_requirement_ (engine_requirement)
  {
  }

private:
  EngineRequirement engine_requirement_;
};

constexpr std::strong_ordering
operator<=> (Command::EngineRequirement lhs, Command::EngineRequirement rhs)
{
  return std::to_underlying (lhs) <=> std::to_underlying (rhs);
}

} // namespace zrythm::commands
