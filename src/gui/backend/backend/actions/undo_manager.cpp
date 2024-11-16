// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/router.h"
#include "common/utils/gtest_wrapper.h"
#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/actions/undo_stack.h"
#include "gui/backend/backend/actions/undoable_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

void
UndoManager::init_loaded ()
{
  undo_stack_->init_loaded ();
  redo_stack_->init_loaded ();
}

void
UndoManager::do_or_undo_action (
  std::unique_ptr<UndoableAction> &&action,
  UndoStack                        &main_stack,
  UndoStack                        &opposite_stack)
{
  // if (ZRYTHM_HAVE_UI)
  //   EVENT_MANAGER->process_now ();

  UndoableAction * action_ptr = action ? action.get () : main_stack.peek ();

  if (&main_stack == undo_stack_.get ())
    action_ptr->undo ();
  else if (&main_stack == redo_stack_.get ())
    action_ptr->perform ();
  else
    z_return_if_reached (); // invalid stack

  if (!action)
    action = main_stack.pop ();

  /* if redo stack is locked don't alter it */
  if (redo_stack_locked_ && &opposite_stack == redo_stack_.get ())
    return;

  /* if the redo stack is full, delete the last element */
  if (opposite_stack.is_full ())
    {
      opposite_stack.pop_last ();

      /* TODO create functions to delete unnecessary files held by the action
       * (eg, something that calls plugin_delete_state_files()) */
    }

  /* push action to the redo stack */
  z_return_if_fail (action);
  opposite_stack.push (std::move (action));

  if (ZRYTHM_TESTING)
    PROJECT->validate ();
}

void
UndoManager::undo ()
{
  z_return_if_fail (!undo_stack_->is_empty ());

  SemaphoreRAII<> sem_guard (action_sem_);

  auto action = undo_stack_->peek ();
  z_return_if_fail (action);
  const auto num_actions = action->num_actions_;
  z_return_if_fail (num_actions > 0);

  for (int i = 0; i < num_actions; ++i)
    {
      z_info ("[ACTION {}/{}]", i + 1, num_actions);
      action = undo_stack_->peek ();
      if (i == 0)
        action->num_actions_ = 1;
      else if (i == num_actions - 1)
        action->num_actions_ = num_actions;

      do_or_undo_action (nullptr, *undo_stack_, *redo_stack_);
    }

  if (ZRYTHM_HAVE_UI)
    {
      /* EVENTS_PUSH (EventType::ET_UNDO_REDO_ACTION_DONE, nullptr); */

      /* process UI events now */
      // EVENT_MANAGER->process_now ();
    }
}

void
UndoManager::redo ()
{
  z_return_if_fail (!redo_stack_->is_empty ());

  SemaphoreRAII<> sem_guard (action_sem_);

  auto action = redo_stack_->peek ();
  z_return_if_fail (action);
  const auto num_actions = action->num_actions_;
  z_return_if_fail (num_actions > 0);

  for (int i = 0; i < num_actions; ++i)
    {
      z_info ("[ACTION {}/{}]", i + 1, num_actions);
      action = redo_stack_->peek ();
      if (i == 0)
        action->num_actions_ = 1;
      else if (i == num_actions - 1)
        action->num_actions_ = num_actions;

      do_or_undo_action (nullptr, *redo_stack_, *undo_stack_);
    }

  if (ZRYTHM_HAVE_UI)
    {
      /* EVENTS_PUSH (EventType::ET_UNDO_REDO_ACTION_DONE, nullptr); */

      /* process UI events now */
      // EVENT_MANAGER->process_now ();
    }
}

void
UndoManager::perform (std::unique_ptr<UndoableAction> &&action)
{
  z_return_if_fail (ROUTER->is_processing_thread () == false);

  /* check that action is not already in the stacks */
  z_return_if_fail (
    !undo_stack_->contains_action (*action)
    && !redo_stack_->contains_action (*action));

  SemaphoreRAII<> sem_guard (action_sem_);

  do_or_undo_action (std::move (action), *redo_stack_, *undo_stack_);

  if (!redo_stack_locked_)
    {
      redo_stack_->clear ();
    }

  if (ZRYTHM_HAVE_UI)
    {
      /* EVENTS_PUSH (EventType::ET_UNDO_REDO_ACTION_DONE, nullptr); */

      /* process UI events now */
      // EVENT_MANAGER->process_now ();
    }
}

bool
UndoManager::contains_clip (const AudioClip &clip) const
{
  return undo_stack_->contains_clip (clip) || redo_stack_->contains_clip (clip);
}

void
UndoManager::get_plugins (std::vector<zrythm::plugins::Plugin *> &plugins) const
{
  undo_stack_->get_plugins (plugins);
  redo_stack_->get_plugins (plugins);
}

UndoableAction *
UndoManager::get_last_action () const
{
  return undo_stack_->peek ();
}

void
UndoManager::clear_stacks ()
{
  undo_stack_->clear ();
  redo_stack_->clear ();
}

void
UndoManager::init_after_cloning (const UndoManager &other)
{
  undo_stack_ = other.undo_stack_->clone_unique ();
  redo_stack_ = other.redo_stack_->clone_unique ();
}