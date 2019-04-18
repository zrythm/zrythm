/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/** \file
 */

#ifndef __AUDIO_ROUTING_H__
#define __AUDIO_ROUTING_H__

#include "utils/sem.h"

typedef enum RouteNodeType
{
  ROUTE_NODE_TYPE_PORT,
  ROUTE_NODE_TYPE_PLUGIN,
} RouteNodeType;

typedef struct RouteNode RouteNode;
typedef struct RouteNode
{
  int         id;

  /* FIXME do we need the srcs? */
  RouteNode * srcs[1200];
  //GArray *    srcs;
  int         refcount;
  int         init_refcount;
  /** AKA refcount. */
  //int         num_srcs;
  RouteNode * dests[1200];
  int         num_dests;
  //GArray *     dests;

  /** Port, if not a plugin or fader. */
  Port * port;

  /** Fader, if fader. */
  //Fader * fader;

  /** Plugin, if plugin. */
  Plugin * pl;

  /**
   * Claimed or not.
   */
  //int         claimed;

  /**
   * Processed or not.
   */
  //int         processed;

  RouteNodeType type;
} RouteNode;

/**
 * Two Routers must be created: one for the initial
 * graph to be re-used and not edited, and one for
 * using on every cycle.
 *
 * When the initial graph is re-calculated, the other
 * one should be destroyed and re-created. This way
 * on every cycle only an update of the refcounts is
 * needed.
 */
typedef struct Router
{
  /** Registry of nodes. */
  //RouteNode * registry[900000];
  //int         num_nodes;
  GArray *    registry;

  /** Hash tables for quick finding of nodes based on
   * the ID of the plugin/port. */
  GHashTable * port_nodes;
  GHashTable * plugin_nodes;

  /** Working trigger nodes to be updated while
   * processing. These are unclaimed. */
  RouteNode * trigger_nodes[6000];
  int         num_trigger_nodes;

  RouteNode * init_trigger_nodes[6000];
  int         num_init_trigger_nodes;

#ifdef HAVE_JACK
  jack_native_thread_t threads[16];
#endif
  int                  num_threads;

  /** Set to 1 when ready to start processing for
   * this cycle. */
  //int                  ready;
  //
  //ZixSem          initing_sem;

  /** Controls triggering work. */
  //ZixSem          trigger_sem;

  /** Controls finishing. */
  ZixSem          finish_sem;

  //ZixSem          route_operation_sem;
  ZixSem          start_cycle_sem;

  int              num_active_threads;

  int         stop_threads;
} Router;

/**
 * Creates a router.
 *
 * Should be used every time the graph is changed.
 */
Router *
router_new ();

/**
 * Initializes as many RT threads as there are cores,
 * -1.
 */
void
router_init_threads (
  Router * router);

/**
 * Starts a new cycle.
 */
void
router_start_cycle (
  Router * cache);

void
router_print (
  Router * router);

void
router_refresh_from (
  Router * router,
  Router * graph);

void
router_destroy (
  Router * router);

#endif
