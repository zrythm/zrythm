// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_selections.h"
#include "actions/channel_send_action.h"
#include "actions/chord_action.h"
#include "actions/midi_mapping_action.h"
#include "actions/mixer_selections_action.h"
#include "actions/port_action.h"
#include "actions/port_connection_action.h"
#include "actions/range_action.h"
#include "actions/tracklist_selections.h"
#include "actions/transport_action.h"
#include "actions/undoable_action.h"
#include "dsp/engine.h"
#include "project.h"
#include "utils/logger.h"
#include "zrythm.h"

#include <glib.h>
#include <glib/gi18n.h>

#include <fmt/format.h>

UndoableAction::
  UndoableAction (Type type, double frames_per_tick, sample_rate_t sample_rate)
    : undoable_action_type_ (type), frames_per_tick_ (frames_per_tick),
      sample_rate_ (sample_rate)
{
  z_return_if_fail (!std::isnan (frames_per_tick));
}

UndoableAction::UndoableAction (Type type)
    : UndoableAction (type, AUDIO_ENGINE->frames_per_tick_, AUDIO_ENGINE->sample_rate_)
{
}

std::unique_ptr<UndoableAction>
UndoableAction::create_unique_from_type (Type type)
{
  switch (type)
    {
    case Type::TracklistSelections:
      return std::make_unique<TracklistSelectionsAction> ();
    case Type::ChannelSend:
      return std::make_unique<ChannelSendAction> ();
    case Type::MixerSelections:
      return std::make_unique<MixerSelectionsAction> ();
    case Type::ArrangerSelections:
      return std::make_unique<ArrangerSelectionsAction> ();
    case Type::MidiMapping:
      return std::make_unique<MidiMappingAction> ();
    case Type::PortConnection:
      return std::make_unique<PortConnectionAction> ();
    case Type::Port:
      return std::make_unique<PortAction> ();
    case Type::Range:
      return std::make_unique<RangeAction> ();
    case Type::Transport:
      return std::make_unique<TransportAction> ();
    case Type::Chord:
      return std::make_unique<ChordAction> ();
    default:
      z_return_val_if_reached (nullptr);
    }
}

void
UndoableAction::init_loaded ()
{
  double sample_rate_ratio =
    (double) AUDIO_ENGINE->sample_rate_ / (double) sample_rate_;
  frames_per_tick_ = frames_per_tick_ * sample_rate_ratio;
  sample_rate_ = AUDIO_ENGINE->sample_rate_;

  /* call the virtual implementation */
  init_loaded_impl ();
}

void
UndoableAction::copy_members_from (const UndoableAction &other)
{
  undoable_action_type_ = other.undoable_action_type_;
  frames_per_tick_ = other.frames_per_tick_;
  sample_rate_ = other.sample_rate_;
  // stack_idx_ = other.stack_idx_;
  num_actions_ = other.num_actions_;
  if (other.port_connections_before_)
    port_connections_before_ = other.port_connections_before_->clone_unique ();
  if (other.port_connections_after_)
    port_connections_after_ = other.port_connections_after_->clone_unique ();
}

void
UndoableAction::do_or_undo (bool perform)
{
  AudioEngine::State state;
  if (needs_pause ())
    {
      /* stop engine and give it some time to stop running */
      AUDIO_ENGINE->wait_for_pause (state, false, true);
    }

  std::string str_to_print =
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
      PROJECT->audio_engine_->transport_->recalculate_total_bars (nullptr);
    }

  if (affects_audio_region_internal_positions ())
    {
      PROJECT->fix_audio_regions ();
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
      port_connections_after_ = PORT_CONNECTIONS_MGR->clone_unique ();
    }
  else if (performing && port_connections_after_ != nullptr)
    {
      z_debug ("resetting port connections from cached after");
      PORT_CONNECTIONS_MGR->reset_connections (port_connections_after_.get ());
      z_return_if_fail (
        PORT_CONNECTIONS_MGR->connections_.size ()
        == port_connections_after_->connections_.size ());
    }
  /* else if undoing and have connections from before */
  else if (!performing && port_connections_before_ != nullptr)
    {
      /* reset the connections */
      z_debug ("resetting port connections from cached before");
      PORT_CONNECTIONS_MGR->reset_connections (port_connections_before_.get ());
      z_return_if_fail (
        PORT_CONNECTIONS_MGR->connections_.size ()
        == port_connections_before_->connections_.size ());
    }
}
