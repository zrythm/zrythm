// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "engine/device_io/engine.h"
#include "gui/backend/backend/actions/arranger_selections_action.h"
#include "gui/backend/backend/actions/channel_send_action.h"
#include "gui/backend/backend/actions/chord_action.h"
#include "gui/backend/backend/actions/midi_mapping_action.h"
#include "gui/backend/backend/actions/mixer_selections_action.h"
#include "gui/backend/backend/actions/port_action.h"
#include "gui/backend/backend/actions/port_connection_action.h"
#include "gui/backend/backend/actions/range_action.h"
#include "gui/backend/backend/actions/tracklist_selections_action.h"
#include "gui/backend/backend/actions/transport_action.h"
#include "gui/backend/backend/actions/undoable_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "utils/logger.h"

#include <fmt/format.h>

namespace zrythm::gui::actions
{

UndoableAction::UndoableAction (
  Type               type,
  dsp::FramesPerTick frames_per_tick,
  sample_rate_t      sample_rate)
    : undoable_action_type_ (type), frames_per_tick_ (frames_per_tick),
      sample_rate_ (sample_rate)
{
}

UndoableAction::UndoableAction (Type type)
    : UndoableAction (
        type,
        AUDIO_ENGINE->frames_per_tick_,
        AUDIO_ENGINE->get_sample_rate ())
{
}

void
UndoableAction::init_loaded (sample_rate_t engine_sample_rate)
{
  double sample_rate_ratio = (double) engine_sample_rate / (double) sample_rate_;
  frames_per_tick_ =
    dsp::FramesPerTick (type_safe::get (frames_per_tick_) * sample_rate_ratio);
  sample_rate_ = engine_sample_rate;

  /* call the virtual implementation */
  init_loaded_impl ();
}

void
init_from (
  UndoableAction        &obj,
  const UndoableAction  &other,
  utils::ObjectCloneType clone_type)
{
  obj.undoable_action_type_ = other.undoable_action_type_;
  obj.frames_per_tick_ = other.frames_per_tick_;
  obj.sample_rate_ = other.sample_rate_;
  // stack_idx_ = other.stack_idx_;
  obj.num_actions_ = other.num_actions_;
  if (other.port_connections_before_)
    obj.port_connections_before_ =
      utils::clone_unique (*other.port_connections_before_);
  if (other.port_connections_after_)
    obj.port_connections_after_ =
      utils::clone_unique (*other.port_connections_after_);
}

void
UndoableAction::do_or_undo (bool perform)
{
  EngineState state{};
  if (needs_pause ())
    {
      /* stop engine and give it some time to stop running */
      AUDIO_ENGINE->wait_for_pause (state, false, true);
    }

  const auto str_to_print =
    fmt::format ("{} ({})", ENUM_NAME (undoable_action_type_), to_string ());
  std::exception_ptr saved_exception = nullptr;
  try
    {
      z_debug ("[DOING ACTION]: {}", str_to_print);
      perform ? perform_impl () : undo_impl ();
      z_debug ("[{}]: {}", perform ? "DONE" : "UNDONE", str_to_print);
    }
  catch (std::exception &e)
    {
      z_warning ("[FAILED]: {}\nReason: {}", str_to_print, e.what ());
      saved_exception = std::current_exception ();
    }

  if (needs_transport_total_bar_update (perform))
    {
      /* recalculate transport bars */
      TRANSPORT->recalculate_total_bars ();
    }

  if (affects_audio_region_internal_positions ())
    {
      // PROJECT->fix_audio_regions ();
    }

  if (needs_pause ())
    {
      /* restart engine */
      AUDIO_ENGINE->resume (state);
    }

  /* rethrow */
  if (saved_exception)
    std::rethrow_exception (saved_exception);
}

void
UndoableAction::perform ()
{
  do_or_undo (true);
}

void
UndoableAction::undo ()
{
  do_or_undo (false);
}

void
UndoableAction::set_num_actions (int num_actions)
{
  z_return_if_fail (num_actions > 0 && num_actions < gZrythm->undo_stack_len_);
  num_actions_ = num_actions;
}

void
UndoableAction::save_or_load_port_connections (bool performing)
{
  /* if first do and keeping track of connections, clone the new connections */
  if (
    performing && port_connections_before_ != nullptr
    && port_connections_after_ == nullptr)
    {
      z_debug ("updating and caching port connections after doing action");
      port_connections_after_ = utils::clone_unique (*PORT_CONNECTIONS_MGR);
    }
  else if (performing && port_connections_after_ != nullptr)
    {
      z_debug ("resetting port connections from cached after");
      PORT_CONNECTIONS_MGR->reset_connections_from_other (
        port_connections_after_.get ());
      z_return_if_fail (
        PORT_CONNECTIONS_MGR->get_connection_count ()
        == port_connections_after_->get_connection_count ());
    }
  /* else if undoing and have connections from before */
  else if (!performing && port_connections_before_ != nullptr)
    {
      /* reset the connections */
      z_debug ("resetting port connections from cached before");
      PORT_CONNECTIONS_MGR->reset_connections_from_other (
        port_connections_before_.get ());
      z_return_if_fail (
        PORT_CONNECTIONS_MGR->get_connection_count ()
        == port_connections_before_->get_connection_count ());
    }
}
}
