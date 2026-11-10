// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "commands/command.h"
#include "undo/journal.h"
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
 *
 * Every history mutation bumps a monotonic revision counter and emits a
 * plain-data journal entry (see set_journal_callback()). Revisions are
 * journal sequence numbers: they are never reused, including after an
 * aborted transaction.
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
  // empty macros never pollute the undo history. While a transaction is
  // open, beginMacro() and endMacro() are refused and the transaction
  // owns the grouping.
  Q_INVOKABLE void beginMacro (const QString &text);
  Q_INVOKABLE void endMacro ();

  /**
   * @brief Returns whether a macro is currently open (begun but not ended).
   *
   * This covers both pending (not yet realized) and realized macros, so it
   * remains accurate even though macro creation is lazy.
   */
  Q_INVOKABLE bool macroActive () const { return open_macro_count_ > 0; }

  /** @brief Current revision; 0 before any mutation. */
  [[nodiscard]] std::uint64_t revision () const noexcept { return revision_; }

  /** @brief Receiver of one journal entry per history mutation. */
  using JournalCallback = std::function<void (const JournalEntry &)>;

  /**
   * @brief Sets the receiver of journal entries.
   *
   * The callback runs synchronously right after each mutation, in
   * mutation order, and must not block or edit the project.
   */
  void set_journal_callback (JournalCallback callback);

  /**
   * @brief An atomic group of pushes with a clean abort.
   *
   * A transaction opens an undo macro under a caller-chosen label.
   * commit() closes that macro, so the whole group becomes one undo
   * step. abort() reverts every command pushed since begin_transaction(),
   * drops them from the history, and records an Abort journal entry;
   * the revision keeps counting forward. Aborting a transaction that
   * received no commands discards it without a journal entry.
   *
   * Transactions may nest; the enclosing transaction is unaffected by
   * an inner commit or abort. While any transaction is open, plain
   * macros and undo/redo are refused.
   *
   * A transaction that is neither committed nor aborted is aborted by
   * its destructor, so early returns and exceptions leave no partial
   * group behind. A transaction must not outlive its stack.
   */
  class Transaction
  {
  public:
    /**
     * @brief Closes the transaction as one undo step.
     *
     * @return false when the transaction already ended or an inner
     * transaction is still open; the transaction then stays open.
     */
    [[nodiscard]] bool commit ();

    /**
     * @brief Reverts the transaction's commands and drops them.
     *
     * @return false when the transaction already ended, an inner
     * transaction is still open, or the engine refused the pause needed
     * to revert; in the last case the transaction stays open with its
     * commands applied, and abort() can be called again.
     */
    [[nodiscard]] bool abort ();

    /** @brief Aborts the transaction unless it already ended. */
    ~Transaction ();

    Q_DISABLE_COPY (Transaction)
    Transaction (Transaction &&other) noexcept;
    // assignment stays deleted: transactions are scope-bound handles
    Transaction &operator= (Transaction &&) = delete;

    [[nodiscard]] bool is_open () const { return state_ == State::Open; }

  private:
    friend class UndoStack;

    Transaction (
      UndoStack    &stack,
      QString       label,
      int           pending_size,
      int           open_macro_count_at_begin,
      int           transaction_depth_at_begin,
      std::uint64_t revision_at_begin);

    enum class State
    {
      Open,
      Committed,
      Aborted,
      MovedFrom,
    };

    UndoStack &stack_;
    QString    label_;
    int        pending_size_;

    /**
     * @brief Open-macro count at begin_transaction() time.
     *
     * Doubles as the index of this transaction's macro in the open-macro
     * stack once realized: every macro open above this transaction
     * (pending or realized) takes a slot first.
     */
    int open_macro_count_at_begin_;

    /** @brief Transaction depth at begin_transaction() time. */
    int transaction_depth_at_begin_;

    std::uint64_t revision_at_begin_;
    State         state_ = State::Open;
  };

  /**
   * @brief Begins a transaction under the given label.
   *
   * @return The transaction, or empty when the stack is executing a
   * command or a plain macro is open.
   */
  std::optional<Transaction> begin_transaction (const QString &label);

  /**
   * @brief Reverts and drops every history entry touched at
   * @p from_revision or later.
   *
   * Still-applied entries are reverted as undo steps first (each
   * journaled as Undo), then the whole suffix is erased and journaled as
   * one Abort entry, so the dropped commands cannot be redone. Entries
   * are dropped whole: an entry is dropped when any push inside it
   * reached @p from_revision.
   *
   * @return false when @p from_revision is not a known revision (0 or
   * above the current one), a macro or transaction is open, or the
   * engine refused a pause needed for a revert; in the last case the
   * already-reverted entries stay undone and redoable, and nothing is
   * erased.
   */
  [[nodiscard]] bool discard_from (std::uint64_t from_revision);

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

  int     count () const { return static_cast<int> (entries_.size ()); }
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

  /**
   * @brief Reverts the most recently applied entry with engine
   * cooperation and journals an Undo entry.
   *
   * @return false when the engine refused the pause; the history is
   * unchanged then.
   */
  bool revert_top_entry ();

  /**
   * @brief Applies the next redoable entry with engine cooperation and
   * journals a Redo entry.
   *
   * @return false when the engine refused the pause; the history is
   * unchanged then.
   */
  bool apply_next_entry ();

  /**
   * @brief Records a completed mutation: bumps the revision and
   * delivers the journal entry.
   */
  void
  record_mutation (JournalEntryKind kind, std::vector<std::string> command_texts);

  /**
   * @brief Closes the innermost open macro, discarding it when it is
   * empty.
   *
   * Shared by endMacro() and Transaction::commit().
   */
  void close_one_open_macro ();

  /**
   * @brief Removes the innermost open realized macro from the history:
   * from its parent macro, or from the root history entries.
   */
  void splice_out_open_macro ();

private:
  /** @brief One root-level history entry: the command plus bookkeeping. */
  struct HistoryEntry
  {
    std::unique_ptr<commands::Command> command;

    /**
     * @brief Revision of the last push that touched this entry.
     *
     * A push into an open root macro updates the open entry, so
     * discard_from() can drop whole entries by revision.
     */
    std::uint64_t last_push_revision = 0;
  };

  /** @brief Done entries; entries above index_ are redoable. */
  std::vector<HistoryEntry> entries_;

  /** @brief Number of done entries; entries_[index_] is next to redo. */
  int index_ = 0;

  /** @brief Maximum history size; 0 means unlimited. */
  int undo_limit_ = 0;

  /** @brief Monotonic mutation counter; see revision(). */
  std::uint64_t revision_ = 0;

  /** @brief Journal receiver; see set_journal_callback(). */
  JournalCallback journal_callback_;

  /** @brief Number of open transactions; see begin_transaction(). */
  int transaction_depth_ = 0;

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
   * @brief Depth of in-progress history activity.
   *
   * A nested push from inside another command's undo()/redo() (e.g. a UI
   * control reacting to a value it restores by pushing an echo edit), or
   * from a change-signal slot firing while the stack is still finishing
   * a mutation, corrupts the command list or the journal order. Pushes,
   * undos, redos, transactions and discards arriving while this is
   * non-zero are refused with an error log instead.
   */
  int executing_depth_ = 0;
};
};
