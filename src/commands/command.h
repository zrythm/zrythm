// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <QString>

namespace zrythm::commands
{

/**
 * @brief Base class for all undoable commands.
 *
 * Commands are owned by the undo stack once pushed and are never copied or
 * moved.
 *
 * Every command states at construction what the audio engine must do around
 * its undo() and redo() when the undo stack runs it. The stack aggregates
 * these declarations across a command and its macro children and cooperates
 * with the engine accordingly.
 */
class Command
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

  virtual ~Command () = default;
  Q_DISABLE_COPY_MOVE (Command)

  /** @brief Reverts the change redo() made. */
  virtual void undo () = 0;

  /** @brief Applies the change. Runs on push and on redo. */
  virtual void redo () = 0;

  /**
   * @brief Attempts to absorb @p other, pushed right after this command.
   *
   * On success this command's redo() must have the same effect as redoing
   * both commands, and its undo() the same effect as undoing both; @p other
   * is then destroyed without its undo() ever running.
   *
   * The stack offers the merge only when this command executed most
   * recently; implementations decide by type what else must match (same
   * object, short time window, ...). The default never merges.
   */
  virtual bool merge_with (const Command &other)
  {
    (void) other;
    return false;
  }

  [[nodiscard]] const QString &text () const noexcept { return text_; }

  /**
   * @brief Engine cooperation this command needs, aggregated over macro
   * children.
   */
  [[nodiscard]] virtual EngineRequirement engine_requirement () const noexcept
  {
    return engine_requirement_;
  }

protected:
  Command (QString text, EngineRequirement engine_requirement)
      : text_ (std::move (text)), engine_requirement_ (engine_requirement)
  {
  }

private:
  QString           text_;
  EngineRequirement engine_requirement_;
};

constexpr std::strong_ordering
operator<=> (Command::EngineRequirement lhs, Command::EngineRequirement rhs)
{
  return std::to_underlying (lhs) <=> std::to_underlying (rhs);
}

/**
 * @brief A command that groups child commands into a single undo/redo step.
 *
 * Created by the undo stack's macro API. Children execute in forward order
 * on redo() and in reverse order on undo().
 */
class MacroCommand final : public Command
{
public:
  explicit MacroCommand (QString text)
      : Command (std::move (text), EngineRequirement::None)
  {
  }

  void undo () override;
  void redo () override;

  [[nodiscard]] EngineRequirement engine_requirement () const noexcept override;

  void add_child (std::unique_ptr<Command> child);

  /**
   * @brief Removes and destroys the most recently added child.
   *
   * @pre The macro has at least one child.
   */
  void remove_last_child ();

  [[nodiscard]] Command *       child_at (std::size_t index) noexcept;
  [[nodiscard]] const Command * child_at (std::size_t index) const noexcept;

  /** @brief The most recently added child, or null when there is none. */
  [[nodiscard]] Command *       last_child () noexcept;
  [[nodiscard]] const Command * last_child () const noexcept;

  [[nodiscard]] std::size_t child_count () const noexcept
  {
    return children_.size ();
  }

private:
  std::vector<std::unique_ptr<Command>> children_;
};

} // namespace zrythm::commands
