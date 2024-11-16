// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2017, 2019 Robin Gareus <robin@gareus.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

#include "zrythm-config.h"

#include "common/dsp/audio_track.h"
#include "common/dsp/control_port.h"
#include "common/dsp/engine.h"
#include "common/utils/debug.h"
#if HAVE_JACK
#  include "common/dsp/engine_jack.h"
#endif
#ifdef HAVE_PORT_AUDIO
#  include "common/dsp/engine_pa.h"
#endif
#include "common/dsp/graph.h"
#include "common/dsp/graph_thread.h"
#include "common/dsp/port.h"
#include "common/dsp/router.h"
#include "common/dsp/tempo_track.h"
#include "common/dsp/track_processor.h"
#include "common/dsp/tracklist.h"
#include "common/utils/flags.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#if HAVE_JACK
#  include "weakjack/weak_libjack.h"
#endif

Router::Router (AudioEngine * engine) : audio_engine_ (engine) { }

nframes_t
Router::get_max_route_playback_latency ()
{
  z_return_val_if_fail (graph_, 0);
  max_route_playback_latency_ = graph_->get_max_route_playback_latency (false);

  return max_route_playback_latency_;
}

void
Router::start_cycle (EngineProcessTimeInfo time_nfo)
{
  z_return_if_fail (graph_);
  z_return_if_fail (
    time_nfo.local_offset_ + time_nfo.nframes_ <= AUDIO_ENGINE->nframes_);

  /* only set the kickoff thread when not called from the gtk thread (sometimes
   * this is called from the gtk thread to force some processing) */
  if (!ZRYTHM_IS_QT_THREAD)
    process_kickoff_thread_ = current_thread_id.get ();

  SemaphoreRAII<> sem (graph_access_sem_);
  if (!sem.is_acquired ())
    {
      z_info ("graph access is busy, returning...");
      return;
    }

  global_offset_ =
    max_route_playback_latency_ - AUDIO_ENGINE->remaining_latency_preroll_;
  time_nfo_ = time_nfo;
  z_return_if_fail_cmp (
    time_nfo.g_start_frame_w_offset_, >=, time_nfo.g_start_frame_);

  /* read control port change events */
  ControlPort::ChangeEvent change{};
  while (ctrl_port_change_queue_.read (change))
    {
      if (ENUM_BITSET_TEST (
            PortIdentifier::Flags, change.flag1, PortIdentifier::Flags::Bpm))
        {
          P_TEMPO_TRACK->set_bpm (change.real_val, 0.f, true, true);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, change.flag2,
          PortIdentifier::Flags2::BeatsPerBar))
        {
          P_TEMPO_TRACK->set_beats_per_bar (change.ival);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, change.flag2, PortIdentifier::Flags2::BeatUnit))
        {
          P_TEMPO_TRACK->set_beat_unit_from_enum (change.beat_unit);
        }
    }

  /* process tempo track ports first */
  if (graph_->bpm_node_)
    {
      graph_->bpm_node_->process (time_nfo, *graph_->main_thread_);
    }
  if (graph_->beats_per_bar_node_)
    {
      graph_->beats_per_bar_node_->process (time_nfo, *graph_->main_thread_);
    }
  if (graph_->beat_unit_node_)
    {
      graph_->beat_unit_node_->process (time_nfo, *graph_->main_thread_);
    }

  callback_in_progress_ = true;
  graph_->callback_start_sem_.release ();
  graph_->callback_done_sem_.acquire ();
  callback_in_progress_ = false;
}

void
Router::recalc_graph (bool soft)
{
  z_info ("Recalculating{}...", soft ? " (soft)" : "");

  if (!graph_ && !soft)
    {
      graph_ = std::make_unique<Graph> (this);
      graph_setup_in_progress_.store (true);
      graph_->setup (true, true);
      graph_setup_in_progress_.store (false);
      graph_->start ();
      return;
    }

  if (soft)
    {
      graph_access_sem_.acquire ();
      graph_->update_latencies (false);
      graph_access_sem_.release ();
    }
  else
    {
      bool running = AUDIO_ENGINE->run_.load ();
      AUDIO_ENGINE->run_.store (false);
      while (AUDIO_ENGINE->cycle_running_.load ())
        {
          std::this_thread::sleep_for (std::chrono::milliseconds (100));
        }
      graph_setup_in_progress_.store (true);
      graph_->setup (true, true);
      graph_setup_in_progress_.store (false);
      AUDIO_ENGINE->run_.store (running);
    }

  z_info ("done");
}

void
Router::queue_control_port_change (const ControlPort::ChangeEvent &change)
{
  ctrl_port_change_queue_.force_write (change);
}