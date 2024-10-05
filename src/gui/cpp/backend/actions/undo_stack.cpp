// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/gtest_wrapper.h"
#include "gui/cpp/backend/actions/arranger_selections.h"
#include "gui/cpp/backend/actions/channel_send_action.h"
#include "gui/cpp/backend/actions/chord_action.h"
#include "gui/cpp/backend/actions/midi_mapping_action.h"
#include "gui/cpp/backend/actions/mixer_selections_action.h"
#include "gui/cpp/backend/actions/port_action.h"
#include "gui/cpp/backend/actions/port_connection_action.h"
#include "gui/cpp/backend/actions/range_action.h"
#include "gui/cpp/backend/actions/tracklist_selections.h"
#include "gui/cpp/backend/actions/transport_action.h"
#include "gui/cpp/backend/actions/undo_stack.h"
#include "gui/cpp/backend/settings/g_settings_manager.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

void
UndoStack::init_loaded ()
{
  for (auto &action : actions_)
    {
      action->init_loaded ();
    }
}

UndoStack::UndoStack ()
{
  const bool testing_or_benchmarking = ZRYTHM_TESTING || ZRYTHM_BENCHMARKING;
  z_return_if_fail (testing_or_benchmarking || G_IS_SETTINGS (S_P_EDITING_UNDO));

  int undo_stack_length =
    testing_or_benchmarking
      ? gZrythm->undo_stack_len_
      : g_settings_get_int (S_P_EDITING_UNDO, "undo-stack-length");
  max_size_ = undo_stack_length;
}

void
UndoStack::init_after_cloning (const UndoStack &other)
{
  max_size_ = other.max_size_;

  /* clone all actions */
  for (auto &action : other.actions_)
    {
      auto variant =
        convert_to_variant<UndoableActionPtrVariant> (action.get ());
      std::visit (
        [&] (auto &&a) { actions_.emplace_back (a->clone_unique ()); }, variant);
    }

  /* init loaded to load the actions on the stack */
  // init_loaded ();
}

std::string
UndoStack::get_as_string (int limit) const
{
  std::string ret;

  int count = std::ssize (actions_) - 1; // Start from the last action
  for (
    auto it = actions_.rbegin ();
    it != actions_.rend () && count >= std::ssize (actions_) - limit;
    ++it, --count)
    {
      auto action_str = (*it)->to_string ();
      ret += fmt::format ("[{}] {}\n", count, action_str);
    }

  return ret;
}