// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Undo stack.
 */

#ifndef __UNDO_UNDO_STACK_H__
#define __UNDO_UNDO_STACK_H__

#include "gui/cpp/backend/actions/undoable_action.h"

#include "common/io/serialization/iserializable.h"
#include "common/utils/icloneable.h"

class AudioClip;

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * Serializable stack for undoable actions.
 *
 * This is used for both undo and redo.
 */
class UndoStack final
    : public ICloneable<UndoStack>,
      public ISerializable<UndoStack>
{
public:
  UndoStack ();

  void init_loaded ();

  /**
   * Gets the list of actions as a string.
   */
  [[nodiscard]] std::string get_as_string (int limit) const;

  /* --- start wrappers --- */

  void push (std::unique_ptr<UndoableAction> &&action)
  {
    if (is_full ())
      {
        pop_last ();
      }
    actions_.push_back (std::move (action));
  }

  std::unique_ptr<UndoableAction> pop ()
  {
    if (actions_.empty ())
      {
        return nullptr;
      }

    auto last_action = std::move (actions_.back ());
    actions_.pop_back ();
    return last_action;
  }

  /**
   * Pops the last (first added) element and moves everything back.
   */
  std::unique_ptr<UndoableAction> pop_last ()
  {
    if (actions_.empty ())
      {
        return nullptr;
      }

    auto last_action = std::move (actions_.front ());
    actions_.pop_front ();
    return last_action;
  }
  auto             size () const { return actions_.size (); }
  bool             is_empty () const { return actions_.empty (); }
  bool             empty () const { return is_empty (); }
  bool             is_full () const { return size () == max_size_; }
  UndoableAction * peek () const
  {
    return is_empty () ? nullptr : actions_.back ().get ();
  }
  void clear () { actions_.clear (); };

  /* --- end wrappers --- */

  [[nodiscard]] bool contains_clip (const AudioClip &clip) const
  {
    return std::any_of (
      actions_.begin (), actions_.end (),
      [&clip] (const auto &action) { return action->contains_clip (clip); });
  }

  /**
   * Checks if the undo stack contains the given
   * action pointer.
   */
  [[nodiscard]] bool contains_action (const UndoableAction &ua) const
  {
    return std::any_of (
      actions_.begin (), actions_.end (),
      [&ua] (const auto &action) { return action.get () == &ua; });
  }

  /**
   * Returns the plugins referred to in the undo stack.
   */
  void get_plugins (std::vector<Plugin *> &arr) const
  {
    for (auto &action : actions_)
      {
        action->get_plugins (arr);
      }
  }

  void init_after_cloning (const UndoStack &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /**
   * @brief Actions on the stack.
   */
  std::deque<std::unique_ptr<UndoableAction>> actions_;

  /**
   * @brief Max size of the stack (if 0, unlimited).
   */
  size_t max_size_ = 0;
};

/**
 * @}
 */

#endif
