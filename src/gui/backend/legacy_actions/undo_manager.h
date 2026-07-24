// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <semaphore>

#include "gui/backend/backend/actions/undo_stack.h"

#define UNDO_MANAGER (PROJECT->undo_manager_)

namespace zrythm::gui::actions
{

/**
 * Undo manager.
 */
class UndoManager : public QObject
{
  Q_OBJECT
  Q_PROPERTY (UndoStack * undoStack READ getUndoStack CONSTANT)
  Q_PROPERTY (UndoStack * redoStack READ getRedoStack CONSTANT)
  QML_ELEMENT

public:
  UndoManager (QObject * parent = nullptr);
  Q_DISABLE_COPY_MOVE (UndoManager)
  ~UndoManager () override = default;

  /**
   * Inits the undo manager by populating the undo/redo stacks.
   */
  void init_loaded (sample_rate_t engine_sample_rate);

  UndoStack * getUndoStack () const { return undo_stack_; }
  UndoStack * getRedoStack () const { return redo_stack_; }

  /**
   * Undo last action.
   *
   * @throw ZrythmException on error.
   */
  Q_INVOKABLE void undo ();

  /**
   * Redo last undone action.
   *
   * @throw ZrythmException on error.
   */
  Q_INVOKABLE void redo ();

  /**
   * Performs the action and pushes it to the undo stack.
   *
   * @note Takes ownership of the action.
   *
   * @throw ZrythmException If the action couldn't be performed.
   */
  Q_INVOKABLE void perform (QObject * action_qobject);

  /**
   * Returns all plugins in the undo stacks.
   *
   * Used when cleaning up state dirs.
   */
  void get_plugins (std::vector<zrythm::plugins::Plugin *> &arr) const;

  /**
   * Returns the last performed action, or NULL if
   * the stack is empty.
   */
  std::optional<UndoableActionPtrVariant> get_last_action () const;

  /**
   * Clears the undo and redo stacks.
   */
  void clear_stacks ();

  friend void init_from (
    UndoManager           &obj,
    const UndoManager     &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr auto kUndoStackKey = "undoStack"sv;
  static constexpr auto kRedoStackKey = "redoStack"sv;
  friend void to_json (nlohmann::json &j, const UndoManager &undo_manager)
  {
    j[kUndoStackKey] = undo_manager.undo_stack_;
  }
  friend void from_json (const nlohmann::json &j, UndoManager &undo_manager)
  {
    j.at (kUndoStackKey).get_to (*undo_manager.undo_stack_);
  }

  /**
   * @brief Does or undoes the given action.
   *
   * @param action An action, when performing, otherwise NULL if undoing/redoing.
   * @param main_stack Undo stack if undoing, redo stack if doing.
   * @throw ZrythmException on error.
   */
  template <UndoableActionSubclass T>
  void
  do_or_undo_action (T * action, UndoStack &main_stack, UndoStack &opposite_stack);

  /**
   * Common logic for undo/redo operations
   * @param is_undo True if undoing, false if redoing
   */
  void do_undo_redo (bool is_undo);

public:
  UndoStack * undo_stack_ = nullptr;
  UndoStack * redo_stack_ = nullptr;

  /**
   * Whether the redo stack is currently locked.
   *
   * This is used as a hack when cancelling arranger drags.
   */
  bool redo_stack_locked_ = false;

  /** Semaphore for performing actions. */
  std::binary_semaphore action_sem_{ 1 };
};

extern template void
UndoManager::do_or_undo_action (
  ArrangerSelectionsAction * action,
  UndoStack                 &main_stack,
  UndoStack                 &opposite_stack);
extern template void
UndoManager::do_or_undo_action (
  ChannelSendAction * action,
  UndoStack          &main_stack,
  UndoStack          &opposite_stack);
extern template void
UndoManager::do_or_undo_action (
  ChordAction * action,
  UndoStack    &main_stack,
  UndoStack    &opposite_stack);
extern template void
UndoManager::do_or_undo_action (
  MidiMappingAction * action,
  UndoStack          &main_stack,
  UndoStack          &opposite_stack);
extern template void
UndoManager::do_or_undo_action (
  MixerSelectionsAction * action,
  UndoStack              &main_stack,
  UndoStack              &opposite_stack);
extern template void
UndoManager::do_or_undo_action (
  PortAction * action,
  UndoStack   &main_stack,
  UndoStack   &opposite_stack);
extern template void
UndoManager::do_or_undo_action (
  PortConnectionAction * action,
  UndoStack             &main_stack,
  UndoStack             &opposite_stack);
extern template void
UndoManager::do_or_undo_action (
  RangeAction * action,
  UndoStack    &main_stack,
  UndoStack    &opposite_stack);
extern template void
UndoManager::do_or_undo_action (
  TracklistSelectionsAction * action,
  UndoStack                  &main_stack,
  UndoStack                  &opposite_stack);
extern template void
UndoManager::do_or_undo_action (
  TransportAction * action,
  UndoStack        &main_stack,
  UndoStack        &opposite_stack);

}; // namespace zrythm::gui::actions
