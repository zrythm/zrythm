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
 */

/**
 * \file
 *
 * Events for calling refresh on widgets.
 *
 * Note: This is only for refreshing widgets. No
 * logic should be performed here. Any logic must be
 * done before pushing an event.
 */
#ifndef __GUI_BACKEND_EVENT_MANAGER_H__
#define __GUI_BACKEND_EVENT_MANAGER_H__

#include "utils/mpmc_queue.h"
#include "utils/object_pool.h"

typedef struct Zrythm Zrythm;
typedef struct ZEvent ZEvent;

/**
 * @addtogroup events
 *
 * @{
 */

/**
 * Event manager.
 */
typedef struct EventManager
{

  /**
   * Event queue, mainly for GUI events.
   */
  MPMCQueue *        mqueue;

  /**
   * Object pool of event structs to avoid real time
   * allocation.
   */
  ObjectPool *       obj_pool;

  /** ID of the event processing source func. */
  guint              process_source_id;

  /** A soft recalculation of the routing graph
   * is pending. */
  bool               pending_soft_recalc;
} EventManager;

#define EVENT_MANAGER (ZRYTHM->event_manager)

/** The event queue. */
#define EVENT_QUEUE (EVENT_MANAGER->mqueue)

#define EVENT_MANAGER_MAX_EVENTS 2000

#define event_queue_push_back_event(q,x) \
  mpmc_queue_push_back (q, (void *) x)

#define event_queue_dequeue_event(q,x) \
  mpmc_queue_dequeue (q, (void *) x)

/**
 * Push events.
 */
#define EVENTS_PUSH(et,_arg) \
  if (ZRYTHM_HAVE_UI && EVENT_MANAGER && \
      EVENT_QUEUE && \
      (!PROJECT || !AUDIO_ENGINE || \
       !AUDIO_ENGINE->exporting)) \
    { \
      ZEvent * _ev = \
        (ZEvent *) \
        object_pool_get (EVENT_MANAGER->obj_pool); \
      _ev->file = __FILE__; \
      _ev->func = __func__; \
      _ev->lineno = __LINE__; \
      _ev->type = et; \
      _ev->arg = (void *) _arg; \
      event_queue_push_back_event (EVENT_QUEUE, _ev); \
    }

/**
 * Creates the event queue and starts the event loop.
 *
 * Must be called from a GTK thread.
 */
EventManager *
event_manager_new (void);

/**
 * Starts accepting events.
 */
void
event_manager_start_events (
  EventManager * self);

/**
 * Stops events from getting fired.
 */
void
event_manager_stop_events (
  EventManager * self);

/**
 * Processes the events now.
 *
 * Must only be called from the GTK thread.
 */
void
event_manager_process_now (
  EventManager * self);

void
event_manager_free (
  EventManager * self);

/**
 * @}
 */

#endif
