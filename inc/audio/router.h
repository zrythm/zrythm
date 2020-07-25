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

/**
 * \file
 *
 * DSP router.
 */

#ifndef __AUDIO_ROUTING_H__
#define __AUDIO_ROUTING_H__

#include "zrythm-config.h"

#include <pthread.h>

#include "utils/types.h"

#include "zix/sem.h"

#include <gtk/gtk.h>

typedef struct GraphNode GraphNode;
typedef struct Graph Graph;
typedef struct PassthroughProcessor
  PassthroughProcessor;
typedef struct MPMCQueue MPMCQueue;
typedef struct Port Port;
typedef struct Fader Fader;
typedef struct Track Track;
typedef struct SampleProcessor SampleProcessor;
typedef struct Plugin Plugin;
typedef struct Position Position;

#ifdef HAVE_JACK
#include "weak_libjack.h"
#endif

/**
 * @addtogroup audio
 *
 * @{
 */

#define ROUTER (AUDIO_ENGINE->router)

typedef struct Router
{
  Graph * graph;

  /** Number of samples to process in this cycle. */
  nframes_t   nsamples;

  /** Stored for the currently processing cycle */
  nframes_t   max_playback_latency;

  /** Current global latency offset (max latency
   * of all routes - remaining latency from
   * engine). */
  nframes_t   global_offset;

  /** Offset in the current cycle. */
  nframes_t   local_offset;

  /** Used when recalculating the graph. */
  ZixSem      graph_access;

} Router;

Router *
router_new (void);

/**
 * Recalculates the process acyclic directed graph.
 *
 * @param soft If true, only readjusts latencies.
 */
void
router_recalc_graph (
  Router * self,
  bool     soft);

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
  const Position * pos);

/**
 * Returns the max playback latency of the trigger
 * nodes.
 */
nframes_t
router_get_max_playback_latency (
  Router * router);

/**
 * Returns if the current thread is a
 * processing thread.
 */
bool
router_is_processing_thread (
  Router * router);

void
router_free (
  Router * self);

/**
 * @}
 */

#endif
