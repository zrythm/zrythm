// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "engine/session/router.h"
#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/actions/undo_stack.h"
#include "gui/backend/backend/actions/undoable_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "utils/gtest_wrapper.h"

namespace zrythm::gui::actions
{

UndoManager::UndoManager (QObject * parent) : QObject (parent)
{
  undo_stack_ = new UndoStack (this);
  redo_stack_ = new UndoStack (this);
}

void
UndoManager::init_loaded (sample_rate_t engine_sample_rate)
{
  undo_stack_->init_loaded (engine_sample_rate);
  redo_stack_->init_loaded (engine_sample_rate);
}

template <UndoableActionSubclass T>
void
UndoManager::do_or_undo_action (
  T *        action,
  UndoStack &main_stack,
  UndoStack &opposite_stack)
{
  // if (ZRYTHM_HAVE_UI)
  //   EVENT_MANAGER->process_now ();

  if (&main_stack == undo_stack_)
    action->undo ();
  else if (&main_stack == redo_stack_)
    action->perform ();
  else
    z_return_if_reached (); // invalid stack

  /* if redo stack is locked don't alter it */
  if (redo_stack_locked_ && &opposite_stack == redo_stack_)
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
  opposite_stack.push (action);
}

void
UndoManager::do_undo_redo (bool is_undo)
{
  auto &main_stack = is_undo ? *undo_stack_ : *redo_stack_;
  auto &opposite_stack = is_undo ? *redo_stack_ : *undo_stack_;

  z_return_if_fail (!main_stack.is_empty ());

  SemaphoreRAII<> sem_guard (action_sem_);

  auto action_opt = main_stack.peek ();
  z_return_if_fail (action_opt.has_value ());

  std::visit (
    [&] (auto &&action) {
      const auto num_actions = action->num_actions_;
      z_return_if_fail (num_actions > 0);

      for (int i = 0; i < num_actions; ++i)
        {
          z_info ("[ACTION {}/{}]", i + 1, num_actions);
          auto action_opt_inner = main_stack.pop ();
          z_return_if_fail (action_opt_inner.has_value ());

          std::visit (
            [&] (auto &&inner_action) {
              if (i == 0)
                inner_action->num_actions_ = 1;
              else if (i == num_actions - 1)
                inner_action->num_actions_ = num_actions;

              do_or_undo_action (inner_action, main_stack, opposite_stack);
            },
            action_opt_inner.value ());
        }

      if (ZRYTHM_HAVE_UI)
        {
          /* EVENTS_PUSH (EventType::ET_UNDO_REDO_ACTION_DONE, nullptr); */
          /* process UI events now */
          // EVENT_MANAGER->process_now ();
        }
    },
    action_opt.value ());
}

void
UndoManager::undo ()
{
  do_undo_redo (true);
}

void
UndoManager::redo ()
{
  do_undo_redo (false);
}

void
UndoManager::perform (QObject * action_qobject)
{
  auto action_var =
    convert_to_variant<UndoableActionPtrVariant> (action_qobject);
  std::visit (
    [&] (auto &&action) {
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
    },
    action_var);
}

void
UndoManager::get_plugins (
  std::vector<zrythm::gui::old_dsp::plugins::Plugin *> &plugins) const
{
  undo_stack_->get_plugins (plugins);
  redo_stack_->get_plugins (plugins);
}

std::optional<UndoableActionPtrVariant>
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
init_from (
  UndoManager           &obj,
  const UndoManager     &other,
  utils::ObjectCloneType clone_type)
{
  obj.undo_stack_ = utils::clone_qobject (*other.undo_stack_, &obj);
  obj.redo_stack_ = utils::clone_qobject (*other.redo_stack_, &obj);
}

template void
UndoManager::do_or_undo_action (
  ArrangerSelectionsAction * action,
  UndoStack                 &main_stack,
  UndoStack                 &opposite_stack);
template void
UndoManager::do_or_undo_action (
  ChannelSendAction * action,
  UndoStack          &main_stack,
  UndoStack          &opposite_stack);
template void
UndoManager::do_or_undo_action (
  ChordAction * action,
  UndoStack    &main_stack,
  UndoStack    &opposite_stack);
template void
UndoManager::do_or_undo_action (
  MidiMappingAction * action,
  UndoStack          &main_stack,
  UndoStack          &opposite_stack);
template void
UndoManager::do_or_undo_action (
  MixerSelectionsAction * action,
  UndoStack              &main_stack,
  UndoStack              &opposite_stack);
template void
UndoManager::do_or_undo_action (
  PortAction * action,
  UndoStack   &main_stack,
  UndoStack   &opposite_stack);
template void
UndoManager::do_or_undo_action (
  PortConnectionAction * action,
  UndoStack             &main_stack,
  UndoStack             &opposite_stack);
template void
UndoManager::do_or_undo_action (
  RangeAction * action,
  UndoStack    &main_stack,
  UndoStack    &opposite_stack);
template void
UndoManager::do_or_undo_action (
  TracklistSelectionsAction * action,
  UndoStack                  &main_stack,
  UndoStack                  &opposite_stack);
template void
UndoManager::do_or_undo_action (
  TransportAction * action,
  UndoStack        &main_stack,
  UndoStack        &opposite_stack);
}
