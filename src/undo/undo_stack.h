// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <memory>
#include <string_view>
#include <vector>

#include "commands/command.h"
#include "utils/inplace_function.h"

#include <QObject>
#include <QStringList>
#include <QtQmlIntegration/qqmlintegration.h>

#include "nlohmann/json_fwd.hpp"

namespace zrythm::undo
{
using namespace std::string_view_literals;

/**
 * @brief Project undo history.
 *
 * Owns the executed commands, tracks the current position for undo/redo,
 * groups commands into macros, and cooperates with the audio engine
 * according to each command's declared engine requirement.
 */
class UndoStack : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QStringList undoActions READ undoActions NOTIFY undoActionsChanged)
  Q_PROPERTY (QStringList redoActions READ redoActions NOTIFY redoActionsChanged)
  Q_PROPERTY (int index READ index WRITE setIndex NOTIFY indexChanged)
  Q_PROPERTY (bool canUndo READ canUndo NOTIFY canUndoChanged)
  Q_PROPERTY (bool canRedo READ canRedo NOTIFY canRedoChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")
public:
  /**
   * @brief A function for requesting a callback with the engine paused.
   *
   * Implementations invoke @p callback synchronously, before returning.
   * When the pause cannot be honored (e.g. an offline render owns
   * processing), they drop the callback instead.
   *
   * @see AudioEngine.execute_function_with_paused_processing_synchronously().
   */
  using CallbackWithPausedEngineRequester =
    std::function<void (utils::InplaceCallback, bool)>;

  explicit UndoStack (
    CallbackWithPausedEngineRequester callback_with_paused_engine_requester,
    QObject *                         parent = nullptr);

  /**
   * @brief Executes @p cmd and adds it to the history.
   *
   * Takes ownership of @p cmd: the command is destroyed when the history
   * is done with it, including when the push is refused.
   */
  void             push (std::unique_ptr<commands::Command> cmd);
  Q_INVOKABLE void undo ();
  Q_INVOKABLE void redo ();

  bool canUndo () const { return open_macro_count_ == 0 && index_ > 0; }
  bool canRedo () const { return open_macro_count_ == 0 && index_ < count (); }

  /**
   * @brief Sets the maximum number of commands kept in the history.
   *
   * Changing the limit never deletes existing entries: raising it applies
   * immediately, and lowering it trims the oldest entries only as new
   * commands arrive. 0 (the default) means no limit.
   */
  void              set_undo_limit (int limit);
  [[nodiscard]] int undo_limit () const { return undo_limit_; }

  const commands::Command * command (int index) const;

  // Macro creation is lazy: beginMacro() only records the macro text, and
  // the macro is realized on the history when the first command is
  // pushed. endMacro() without any intervening push() discards the macro, so
  // empty macros never pollute the undo history.
  Q_INVOKABLE void beginMacro (const QString &text);
  Q_INVOKABLE void endMacro ();

  /**
   * @brief Returns whether a macro is currently open (begun but not ended).
   *
   * This covers both pending (not yet realized) and realized macros, so it
   * remains accurate even though macro creation is lazy.
   */
  Q_INVOKABLE bool macroActive () const { return open_macro_count_ > 0; }

  /**
   * @brief RAII helper that ends an undo macro on destruction.
   *
   * Keeps beginMacro()/endMacro() balanced on early returns and exceptions.
   * Because macro creation is lazy (see beginMacro()), destructing without
   * any pushes discards the macro instead of pushing an empty one.
   */
  class ScopedMacro
  {
  public:
    ScopedMacro (UndoStack &stack, const QString &text) : stack_ (stack)
    {
      stack_.beginMacro (text);
    }
    ~ScopedMacro () { stack_.endMacro (); }
    Q_DISABLE_COPY_MOVE (ScopedMacro)

  private:
    UndoStack &stack_;
  };

  QStringList undoActions ();
  QStringList redoActions ();

  int  index () const { return index_; }
  void setIndex (int idx);

  int     count () const { return static_cast<int> (commands_.size ()); }
  QString text (int idx) const;

Q_SIGNALS:
  void undoActionsChanged ();
  void redoActionsChanged ();
  void indexChanged ();
  void canUndoChanged ();
  void canRedoChanged ();

private:
  // persistence (TODO)
  static constexpr auto kStackContentKey = "stackContent"sv;
  friend void           to_json (nlohmann::json &j, const UndoStack &u);
  friend void           from_json (const nlohmann::json &j, UndoStack &u);

  /**
   * @brief Runs @p action with the engine cooperation @p requirement asks
   * for.
   *
   * Actions needing no cooperation run directly; every other action is
   * handed to the pause requester, which either runs it or drops it.
   */
  void dispatch_with_engine_cooperation (
    commands::Command::EngineRequirement requirement,
    utils::InplaceCallback               action);

  /**
   * @brief Runs @p action with the engine cooperation the command (or any
   * of its macro children) declares via
   * commands::Command::engine_requirement().
   */
  void execute_with_engine_pause_if_needed (
    const commands::Command &cmd,
    utils::InplaceCallback   action);

  /**
   * @brief Adds an open macro with the given text to the history.
   *
   * A root macro discards the redo history above the current index; a
   * nested macro becomes a child of the currently open macro. The history
   * index advances when the root macro is ended, not here.
   */
  void begin_macro_core (const QString &text);

  /** @brief Closes the innermost open realized macro. */
  void end_macro_core ();

  /**
   * @brief Executes @p cmd and merges or appends it (the core of push()).
   *
   * Takes over @p cmd: runs its redo(), then offers it to the most
   * recently executed command for merging — the innermost open macro's
   * last child while a macro is open, the command below the history index
   * otherwise. On a successful merge @p cmd is destroyed; otherwise it is
   * appended to the history.
   */
  void push_core (std::unique_ptr<commands::Command> cmd);

  /** @brief Deletes the oldest history entries beyond the undo limit. */
  void check_undo_limit ();

  /** @brief Emits all history change signals. */
  void emit_changed ();

private:
  /** @brief Done commands; entries above index_ are redoable. */
  std::vector<std::unique_ptr<commands::Command>> commands_;

  /** @brief Number of done commands; commands_[index_] is next to redo. */
  int index_ = 0;

  /** @brief Maximum history size; 0 means unlimited. */
  int undo_limit_ = 0;

  /** @brief Open realized macros, outermost first (non-owning). */
  std::vector<commands::MacroCommand *> macro_stack_;

  /**
   * Texts of macros begun via beginMacro() but not yet realized on the
   * history (see beginMacro()). Realized in order on the next
   * push(); pending entries nest inside any already-realized macro.
   */
  std::vector<QString> pending_macros_;

  // Number of macros begun but not yet ended (pending + realized)
  int open_macro_count_ = 0;

  // Engine operations
  CallbackWithPausedEngineRequester callback_with_paused_engine_requester_;

  /**
   * @brief True while the history is executing a push, undo or redo.
   *
   * A nested push from inside another command's undo()/redo() (e.g. a UI
   * control reacting to a value it restores by pushing an echo edit)
   * corrupts the command list. Pushes, undos and redos arriving while this
   * is set are refused with an error log instead.
   */
  bool executing_ = false;
};
};
