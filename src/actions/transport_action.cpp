// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/transport_action.h"
#include "dsp/control_port.h"
#include "dsp/engine.h"
#include "dsp/router.h"
#include "dsp/tempo_track.h"
#include "dsp/tracklist.h"
#include "dsp/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

TransportAction::
  TransportAction (bpm_t bpm_before, bpm_t bpm_after, bool already_done)
    : type_ (Type::TempoChange), bpm_before_ (bpm_before),
      bpm_after_ (bpm_after), already_done_ (already_done),
      musical_mode_ (
        ZRYTHM_TESTING ? false : g_settings_get_boolean (S_UI, "musical-mode"))
{
  TransportAction ();
}

TransportAction::
  TransportAction (Type type, int before, int after, bool already_done)
    : type_ (type), int_before_ (before), int_after_ (after),
      already_done_ (already_done),
      musical_mode_ (
        ZRYTHM_TESTING ? false : g_settings_get_boolean (S_UI, "musical-mode"))
{
  TransportAction ();
}

void
TransportAction::do_or_undo (bool do_it)
{
  ControlPort::ChangeEvent change = {};
  switch (type_)
    {
    case Type::TempoChange:
      change.flag1 = PortIdentifier::Flags::Bpm;
      change.real_val = do_it ? bpm_after_ : bpm_before_;
      break;
    case Type::BeatsPerBarChange:
      change.flag2 = PortIdentifier::Flags2::BeatsPerBar;
      change.ival = do_it ? int_after_ : int_before_;
      break;
    case Type::BeatUnitChange:
      change.flag2 = PortIdentifier::Flags2::BeatUnit;
      change.beat_unit =
        TempoTrack::beat_unit_to_enum (do_it ? int_after_ : int_before_);
      break;
    }

  /* queue change */
  ROUTER->queue_control_port_change (change);

  /* run engine to apply the change */
  AUDIO_ENGINE->process_prepare (1);
  EngineProcessTimeInfo time_nfo = {
    .g_start_frame_ = (unsigned_frame_t) PLAYHEAD.frames_,
    .g_start_frame_w_offset_ = (unsigned_frame_t) PLAYHEAD.frames_,
    .local_offset_ = 0,
    .nframes_ = 1,
  };
  ROUTER->start_cycle (time_nfo);
  AUDIO_ENGINE->post_process (0, 1);

  int  beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
  bool update_from_ticks = need_update_positions_from_ticks ();
  AUDIO_ENGINE->update_frames_per_tick (
    beats_per_bar, P_TEMPO_TRACK->get_current_bpm (),
    AUDIO_ENGINE->sample_rate_, true, update_from_ticks, false);

  if (type_ == Type::TempoChange)
    {
      /* get time ratio */
      double time_ratio = 0;
      if (do_it)
        {
          time_ratio = bpm_after_ / bpm_before_;
        }
      else
        {
          time_ratio = bpm_before_ / bpm_after_;
        }

      if (musical_mode_ && false) // doesn't work
        {
          TRANSPORT->stretch_regions (nullptr, true, time_ratio, false);
        }
    }
}

void
TransportAction::perform_impl ()
{
  if (already_done_)
    {
      already_done_ = false;

      int   beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
      bpm_t bpm = P_TEMPO_TRACK->get_current_bpm ();

      bool update_from_ticks = need_update_positions_from_ticks ();
      AUDIO_ENGINE->update_frames_per_tick (
        beats_per_bar, bpm, AUDIO_ENGINE->sample_rate_, true, update_from_ticks,
        false);
    }
  else
    {
      do_or_undo (true);
      TRACKLIST->set_caches (CacheType::PlaybackSnapshots);
    }

  EVENTS_PUSH (EventType::ET_BPM_CHANGED, nullptr);
  EVENTS_PUSH (EventType::ET_TIME_SIGNATURE_CHANGED, nullptr);
}

void
TransportAction::undo_impl ()
{
  do_or_undo (false);

  EVENTS_PUSH (EventType::ET_BPM_CHANGED, nullptr);
  EVENTS_PUSH (EventType::ET_TIME_SIGNATURE_CHANGED, nullptr);
}

std::string
TransportAction::to_string () const
{
  switch (type_)
    {
    case Type::TempoChange:
      return format_str (
        _ ("Change BPM from {:.2f} to {:.2f}"), bpm_before_, bpm_after_);
    case Type::BeatsPerBarChange:
      return format_str (
        _ ("Change beats per bar from {} to {}"), int_before_, int_after_);
    case Type::BeatUnitChange:
      return format_str (
        _ ("Change beat unit from {} to {}"), int_before_, int_after_);
    }

  z_return_val_if_reached ("");
}