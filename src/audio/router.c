/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (C) 2017, 2019 Robin Gareus <robin@gareus.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm-config.h"

#include "audio/audio_track.h"
#include "audio/engine.h"
#include "audio/engine_alsa.h"
#ifdef HAVE_JACK
#include "audio/engine_jack.h"
#endif
#ifdef HAVE_PORT_AUDIO
#include "audio/engine_pa.h"
#endif
#include "audio/graph.h"
#include "audio/graph_thread.h"
#include "audio/master_track.h"
#include "audio/midi.h"
#include "audio/midi_track.h"
#include "audio/modulator.h"
#include "audio/pan.h"
#include "audio/port.h"
#include "audio/router.h"
#include "audio/stretcher.h"
#include "audio/track.h"
#include "audio/track_processor.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/env.h"
#include "utils/mpmc_queue.h"
#include "utils/object_utils.h"
#include "utils/objects.h"
#include "utils/stoat.h"

#ifdef HAVE_JACK
#include "weak_libjack.h"
#endif

/**
 * Returns the max playback latency of the trigger
 * nodes.
 */
nframes_t
router_get_max_playback_latency (
  Router * router)
{
  g_return_val_if_fail (
    router && router->graph, 0);
  router->max_playback_latency =
    graph_get_max_playback_latency (
      router->graph);

  return router->max_playback_latency;
}

/**
 * Starts a new cycle.
 *
 * @param local_offset The local offset to start
 *   playing from in this cycle:
 *   (0 - <engine buffer size>)
 */
void
router_start_cycle (
  Router *         self,
  const nframes_t  nsamples,
  const nframes_t  local_offset,
  const Position * pos)
{
  g_return_if_fail (self && self->graph);

  if (!zix_sem_try_wait (&self->graph_access))
    return;

  self->nsamples = nsamples;
  self->global_offset =
    self->max_playback_latency -
    AUDIO_ENGINE->remaining_latency_preroll;
  self->local_offset = local_offset;
  zix_sem_post (&self->graph->callback_start);
  zix_sem_wait (&self->graph->callback_done);

  zix_sem_post (&self->graph_access);
}

/**
 * Recalculates the process acyclic directed graph.
 *
 * @param soft If true, only readjusts latencies.
 */
void
router_recalc_graph (
  Router * self,
  bool     soft)
{
  g_message ("Recalculating...");

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
      zix_sem_wait (&self->graph_access);
      graph_setup (self->graph, 1, 1);
      zix_sem_post (&self->graph_access);
    }

  g_message ("done");
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

  g_message ("done");

  return self;
}

/**
 * Returns if the current thread is a
 * processing thread.
 */
bool
router_is_processing_thread (
  Router * self)
{
  if (!self || !self->graph)
    return false;

  for (int j = 0;
       j < self->graph->num_threads; j++)
    {
#ifdef HAVE_JACK
      if (AUDIO_ENGINE->audio_backend ==
            AUDIO_BACKEND_JACK)
        {
          if (pthread_equal (
                pthread_self (),
                self->graph->threads[j]->jthread))
            return true;
        }
#endif
#ifndef HAVE_JACK
      if (pthread_equal (
            pthread_self (),
            self->graph->threads[j]->pthread))
        return true;
#endif
    }

#ifdef HAVE_JACK
  if (AUDIO_ENGINE->audio_backend ==
        AUDIO_BACKEND_JACK)
    {
      if (pthread_equal (
            pthread_self (),
            self->graph->main_thread->jthread))
        return true;
    }
#endif
#ifndef HAVE_JACK
  if (pthread_equal (
        pthread_self (),
        self->graph->main_thread->pthread))
    return true;
#endif

  return false;
}

void
router_free (
  Router * self)
{
  g_message ("%s: freeing...", __func__);

  if (self->graph)
    graph_destroy (self->graph);
  self->graph = NULL;

  zix_sem_destroy (&self->graph_access);
  object_set_to_zero (&self->graph_access);

  object_zero_and_free (self);

  g_message ("%s: done", __func__);
}
