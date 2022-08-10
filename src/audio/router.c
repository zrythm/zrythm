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

#include "audio/audio_track.h"
#include "audio/control_port.h"
#include "audio/engine.h"
#include "audio/engine_alsa.h"
#ifdef HAVE_JACK
#  include "audio/engine_jack.h"
#endif
#ifdef HAVE_PORT_AUDIO
#  include "audio/engine_pa.h"
#endif
#include "audio/graph.h"
#include "audio/graph_thread.h"
#include "audio/master_track.h"
#include "audio/midi_track.h"
#include "audio/pan.h"
#include "audio/port.h"
#include "audio/router.h"
#include "audio/stretcher.h"
#include "audio/tempo_track.h"
#include "audio/track.h"
#include "audio/track_processor.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/env.h"
#include "utils/flags.h"
#include "utils/midi.h"
#include "utils/mpmc_queue.h"
#include "utils/object_utils.h"
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
router_start_cycle (
  Router *              self,
  EngineProcessTimeInfo time_nfo)
{
  g_return_if_fail (self && self->graph);
  g_return_if_fail (
    time_nfo.local_offset + time_nfo.nframes
    <= AUDIO_ENGINE->nframes);

  /* only set the kickoff thread when not called
   * from the gtk thread (sometimes this is called
   * from the gtk thread to force some
   * processing) */
  if (g_thread_self () != zrythm_app->gtk_thread)
    self->process_kickoff_thread = g_thread_self ();

  if (!zix_sem_try_wait (&self->graph_access))
    {
      g_message ("graph access is busy, returning...");
      return;
    }

  self->global_offset =
    self->max_route_playback_latency
    - AUDIO_ENGINE->remaining_latency_preroll;
  memcpy (
    &self->time_nfo, &time_nfo,
    sizeof (EngineProcessTimeInfo));

  /* read control port change events */
  while (
    zix_ring_read_space (self->ctrl_port_change_queue)
    >= sizeof (ControlPortChange))
    {
      ControlPortChange change = { 0 };
      zix_ring_read (
        self->ctrl_port_change_queue, &change,
        sizeof (change));
      if (change.flag1 & PORT_FLAG_BPM)
        {
          tempo_track_set_bpm (
            P_TEMPO_TRACK, change.real_val, 0.f, true,
            F_PUBLISH_EVENTS);
        }
      else if (change.flag2 & PORT_FLAG2_BEATS_PER_BAR)
        {
          tempo_track_set_beats_per_bar (
            P_TEMPO_TRACK, change.ival);
        }
      else if (change.flag2 & PORT_FLAG2_BEAT_UNIT)
        {
          tempo_track_set_beat_unit_from_enum (
            P_TEMPO_TRACK, change.beat_unit);
        }
    }

  /* process tempo track ports first */
  if (self->graph->bpm_node)
    {
      graph_node_process (self->graph->bpm_node, time_nfo);
    }
  if (self->graph->beats_per_bar_node)
    {
      graph_node_process (
        self->graph->beats_per_bar_node, time_nfo);
    }
  if (self->graph->beat_unit_node)
    {
      graph_node_process (
        self->graph->beat_unit_node, time_nfo);
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
      graph_setup (self->graph, 1, 1);
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
      graph_setup (self->graph, 1, 1);
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
router_queue_control_port_change (
  Router *                  self,
  const ControlPortChange * change)
{
  if (
    zix_ring_write_space (self->ctrl_port_change_queue)
    < sizeof (ControlPortChange))
    {
      zix_ring_skip (
        self->ctrl_port_change_queue,
        sizeof (ControlPortChange));
    }

  zix_ring_write (
    self->ctrl_port_change_queue, change,
    sizeof (ControlPortChange));
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

  self->ctrl_port_change_queue =
    zix_ring_new (zix_default_allocator (), sizeof (ControlPortChange) * (size_t) 24);

  g_message ("done");

  return self;
}

/**
 * Returns whether this is the thread that kicks
 * off processing (thread that calls
 * router_start_cycle()).
 */
bool
router_is_processing_kickoff_thread (const Router * const self)
{
  return g_thread_self () == self->process_kickoff_thread;
}

/**
 * Returns if the current thread is a
 * processing thread.
 */
bool
router_is_processing_thread (const Router * const self)
{
  if (!self->graph)
    return false;

  for (int j = 0; j < self->graph->num_threads; j++)
    {
      if (pthread_equal (
            pthread_self (), self->graph->threads[j]->pthread))
        return true;
    }

  if (
    self->graph->main_thread
    && pthread_equal (
      pthread_self (), self->graph->main_thread->pthread))
    return true;

  return false;
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

  object_free_w_func_and_null (
    zix_ring_free, self->ctrl_port_change_queue);

  object_zero_and_free (self);

  g_debug ("%s: done", __func__);
}
