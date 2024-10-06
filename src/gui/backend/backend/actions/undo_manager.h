// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UNDO_UNDO_MANAGER_H__
#define __UNDO_UNDO_MANAGER_H__

#include <semaphore>

#include "gui/backend/backend/actions/undo_stack.h"

class AudioClip;

/**
 * @addtogroup actions
 *
 * @{
 */

#define UNDO_MANAGER (PROJECT->undo_manager_)

/**
 * Undo manager.
 */
class UndoManager final
    : public ICloneable<UndoManager>,
      public ISerializable<UndoManager>
{
public:
  UndoManager ()
  {
    undo_stack_ = std::make_unique<UndoStack> ();
    redo_stack_ = std::make_unique<UndoStack> ();
  }

  /**
   * Inits the undo manager by populating the undo/redo stacks.
   */
  void init_loaded ();

  /**
   * Undo last action.
   *
   * @throw ZrythmException on error.
   */
  void undo ();

  /**
   * Redo last undone action.
   *
   * @throw ZrythmException on error.
   */
  void redo ();

  /**
   * Performs the action and pushes it to the undo stack.
   *
   * @throw ZrythmException If the action couldn't be performed.
   */
  void perform (std::unique_ptr<UndoableAction> &&action);

  /**
   * Returns whether the given clip is used by any
   * stack.
   */
  bool contains_clip (const AudioClip &clip) const;

  /**
   * Returns all plugins in the undo stacks.
   *
   * Used when cleaning up state dirs.
   */
  void get_plugins (std::vector<Plugin *> &arr) const;

  /**
   * Returns the last performed action, or NULL if
   * the stack is empty.
   */
  UndoableAction * get_last_action () const;

  /**
   * Clears the undo and redo stacks.
   */
  void clear_stacks ();

  void init_after_cloning (const UndoManager &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  /**
   * @brief Does or undoes the given action.
   *
   * @param action An action, when performing, otherwise NULL if undoing/redoing.
   * @param main_stack Undo stack if undoing, redo stack if doing.
   * @throw ZrythmException on error.
   */
  void do_or_undo_action (
    std::unique_ptr<UndoableAction> &&action,
    UndoStack                        &main_stack,
    UndoStack                        &opposite_stack);

public:
  std::unique_ptr<UndoStack> undo_stack_;
  std::unique_ptr<UndoStack> redo_stack_;

  /**
   * Whether the redo stack is currently locked.
   *
   * This is used as a hack when cancelling arranger drags.
   */
  bool redo_stack_locked_ = false;

  /** Semaphore for performing actions. */
  std::binary_semaphore action_sem_{ 1 };
};

/**
 * @}
 */

#endif
