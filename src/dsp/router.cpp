// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
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

#ifdef HAVE_C11_THREADS
#  include <threads.h>
#endif

#include "dsp/audio_track.h"
#include "dsp/control_port.h"
#include "dsp/engine.h"
#include "dsp/engine_alsa.h"
#include "utils/debug.h"
#ifdef HAVE_JACK
#  include "dsp/engine_jack.h"
#endif
#ifdef HAVE_PORT_AUDIO
#  include "dsp/engine_pa.h"
#endif
#include "dsp/graph.h"
#include "dsp/graph_thread.h"
#include "dsp/master_track.h"
#include "dsp/midi_track.h"
#include "dsp/pan.h"
#include "dsp/port.h"
#include "dsp/router.h"
#include "dsp/stretcher.h"
#include "dsp/tempo_track.h"
#include "dsp/track.h"
#include "dsp/track_processor.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/env.h"
#include "utils/flags.h"
#include "utils/midi.h"
#include "utils/mpmc_queue.h"
#include "utils/objects.h"
#include "utils/stoat.h"
#include "zrythm_app.h"

#ifdef HAVE_JACK
#  include "weak_libjack.h"
#endif

/**
 * Returns the max playback latency of the trigger
 * nodes.
 */
nframes_t
router_get_max_route_playback_latency (Router * router)
{
  g_return_val_if_fail (router && router->graph, 0);
  router->max_route_playback_latency =
    graph_get_max_route_playback_latency (router->graph, false);

  return router->max_route_playback_latency;
}

/**
 * Starts a new cycle.
 */
void
router_start_cycle (Router * self, EngineProcessTimeInfo time_nfo)
{
  g_return_if_fail (self && self->graph);
  g_return_if_fail (
    time_nfo.local_offset + time_nfo.nframes <= AUDIO_ENGINE->nframes);

  /* only set the kickoff thread when not called from the gtk thread (sometimes
   * this is called from the gtk thread to force some processing) */
  if (g_thread_self () != zrythm_app->gtk_thread)
    self->process_kickoff_thread = g_thread_self ();

  if (zix_sem_try_wait (&self->graph_access) != ZIX_STATUS_SUCCESS)
    {
      g_message ("graph access is busy, returning...");
      return;
    }

  self->global_offset =
    self->max_route_playback_latency - AUDIO_ENGINE->remaining_latency_preroll;
  memcpy (&self->time_nfo, &time_nfo, sizeof (EngineProcessTimeInfo));
  z_return_if_fail_cmp (
    time_nfo.g_start_frame_w_offset, >=, time_nfo.g_start_frame);

  /* read control port change events */
  while (
    zix_ring_read_space (self->ctrl_port_change_queue)
    >= sizeof (ControlPortChange))
    {
      ControlPortChange change = {};
      zix_ring_read (self->ctrl_port_change_queue, &change, sizeof (change));
      if (
        ENUM_BITSET_TEST (ZPortFlags, change.flag1, ZPortFlags::Z_PORT_FLAG_BPM))
        {
          tempo_track_set_bpm (
            P_TEMPO_TRACK, change.real_val, 0.f, true, F_PUBLISH_EVENTS);
        }
      else if (
        ENUM_BITSET_TEST (
          ZPortFlags2, change.flag2, ZPortFlags2::Z_PORT_FLAG2_BEATS_PER_BAR))
        {
          tempo_track_set_beats_per_bar (P_TEMPO_TRACK, change.ival);
        }
      else if (
        ENUM_BITSET_TEST (
          ZPortFlags2, change.flag2, ZPortFlags2::Z_PORT_FLAG2_BEAT_UNIT))
        {
          tempo_track_set_beat_unit_from_enum (P_TEMPO_TRACK, change.beat_unit);
        }
    }

  /* process tempo track ports first */
  if (self->graph->bpm_node)
    {
      graph_node_process (self->graph->bpm_node, time_nfo);
    }
  if (self->graph->beats_per_bar_node)
    {
      graph_node_process (self->graph->beats_per_bar_node, time_nfo);
    }
  if (self->graph->beat_unit_node)
    {
      graph_node_process (self->graph->beat_unit_node, time_nfo);
    }

  self->callback_in_progress = true;
  zix_sem_post (&self->graph->callback_start);
  zix_sem_wait (&self->graph->callback_done);
  self->callback_in_progress = false;

  zix_sem_post (&self->graph_access);
}

/**
 * Recalculates the process acyclic directed graph.
 *
 * @param soft If true, only readjusts latencies.
 */
void
router_recalc_graph (Router * self, bool soft)
{
  g_message ("Recalculating%s...", soft ? " (soft)" : "");

  g_return_if_fail (self);

  if (!self->graph && !soft)
    {
      self->graph = graph_new (self);
      g_atomic_int_set (&self->graph_setup_in_progress, 1);
      graph_setup (self->graph, 1, 1);
      g_atomic_int_set (&self->graph_setup_in_progress, 0);
      graph_start (self->graph);
      return;
    }

  if (soft)
    {
      zix_sem_wait (&self->graph_access);
      graph_update_latencies (self->graph, false);
      zix_sem_post (&self->graph_access);
    }
  else
    {
      int running = g_atomic_int_get (&AUDIO_ENGINE->run);
      g_atomic_int_set (&AUDIO_ENGINE->run, 0);
      while (g_atomic_int_get (&AUDIO_ENGINE->cycle_running))
        {
          g_usleep (100);
        }
      g_atomic_int_set (&self->graph_setup_in_progress, 1);
      graph_setup (self->graph, 1, 1);
      g_atomic_int_set (&self->graph_setup_in_progress, 0);
      g_atomic_int_set (&AUDIO_ENGINE->run, (guint) running);
    }

  g_message ("done");
}

/**
 * Queues a control port change to be applied
 * when processing starts.
 *
 * Currently only applies to BPM/time signature
 * changes.
 */
void
router_queue_control_port_change (Router * self, const ControlPortChange * change)
{
  if (
    zix_ring_write_space (self->ctrl_port_change_queue)
    < sizeof (ControlPortChange))
    {
      zix_ring_skip (self->ctrl_port_change_queue, sizeof (ControlPortChange));
    }

  zix_ring_write (
    self->ctrl_port_change_queue, change, sizeof (ControlPortChange));
}

/**
 * Creates a new Router.
 *
 * There is only 1 router needed in a project.
 */
Router *
router_new (void)
{
  g_message ("Creating new router...");

  Router * self = object_new (Router);

  zix_sem_init (&self->graph_access, 1);

  self->ctrl_port_change_queue = zix_ring_new (
    zix_default_allocator (), sizeof (ControlPortChange) * (size_t) 24);

  g_message ("done");

  return self;
}

void
router_free (Router * self)
{
  g_debug ("%s: freeing...", __func__);

  if (self->graph)
    graph_destroy (self->graph);
  self->graph = NULL;

  zix_sem_destroy (&self->graph_access);
  object_set_to_zero (&self->graph_access);

  object_free_w_func_and_null (zix_ring_free, self->ctrl_port_change_queue);

  object_zero_and_free (self);

  g_debug ("%s: done", __func__);
}
