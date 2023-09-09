// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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

#include "utils/backtrace.h"
#include "utils/mpmc_queue.h"
#include "utils/object_pool.h"

#include <glib.h>

typedef struct Zrythm Zrythm;
typedef struct ZEvent ZEvent;

/**
 * @addtogroup events
 *
 * @{
 */

/**
 * Event manager for the UI.
 *
 * This API is responsible for collecting and processing UI
 * events on the GTK thread.
 *
 * @see ZEvent.
 */
typedef struct EventManager
{
  /**
   * Event queue, mainly for GUI events.
   */
  MPMCQueue * mqueue;

  /**
   * Object pool of event structs to avoid real time
   * allocation.
   */
  ObjectPool * obj_pool;

  /**
   * ID of the event processing source func.
   *
   * Will be zero when stopped and non-zero when active.
   */
  guint process_source_id;

  /** A soft recalculation of the routing graph
   * is pending. */
  bool pending_soft_recalc;

  /** Events array to use during processing. */
  GPtrArray * events_arr;
} EventManager;

#define EVENT_MANAGER (ZRYTHM->event_manager)

/** The event queue. */
#define EVENT_QUEUE (EVENT_MANAGER->mqueue)

#define EVENT_MANAGER_MAX_EVENTS 4000

#define event_queue_push_back_event(q, x) mpmc_queue_push_back (q, (void *) x)

#define event_queue_dequeue_event(q, x) mpmc_queue_dequeue (q, (void *) x)

/**
 * Push events.
 */
#define EVENTS_PUSH(et, _arg) \
  if ( \
    ZRYTHM_HAVE_UI && EVENT_MANAGER && EVENT_QUEUE \
    && (!PROJECT || !AUDIO_ENGINE || !AUDIO_ENGINE->exporting) \
    && EVENT_MANAGER->process_source_id) \
    { \
      ZEvent * _ev = (ZEvent *) object_pool_get (EVENT_MANAGER->obj_pool); \
      _ev->file = __FILE__; \
      _ev->func = __func__; \
      _ev->lineno = __LINE__; \
      _ev->type = (et); \
      _ev->arg = (void *) (_arg); \
      if ( \
        zrythm_app->gtk_thread == g_thread_self () /* skip backtrace for now */ \
        && false) \
        { \
          _ev->backtrace = backtrace_get ("", 40, false); \
        } \
      /* don't print events that are called \
       * continuously */ \
      if ( \
        (et) != ET_PLAYHEAD_POS_CHANGED \
        && g_thread_self () == zrythm_app->gtk_thread) \
        { \
          g_debug ("pushing UI event " #et " (%s:%d)", __func__, __LINE__); \
        } \
      event_queue_push_back_event (EVENT_QUEUE, _ev); \
    }

/* runs the event logic now */
#define EVENTS_PUSH_NOW(et, _arg) \
  if ( \
    ZRYTHM_HAVE_UI && EVENT_MANAGER && EVENT_QUEUE \
    && zrythm_app->gtk_thread == g_thread_self () \
    && (!PROJECT || !AUDIO_ENGINE || !AUDIO_ENGINE->exporting) \
    && EVENT_MANAGER->process_source_id) \
    { \
      ZEvent * _ev = (ZEvent *) object_pool_get (EVENT_MANAGER->obj_pool); \
      _ev->file = __FILE__; \
      _ev->func = __func__; \
      _ev->lineno = __LINE__; \
      _ev->type = et; \
      _ev->arg = (void *) _arg; \
      if (/* skip backtrace for now */ \
          false) \
        { \
          _ev->backtrace = backtrace_get ("", 40, false); \
        } \
      /* don't print events that are called \
       * continuously */ \
      if (et != ET_PLAYHEAD_POS_CHANGED) \
        { \
          g_debug ( \
            "processing UI event now " #et " (%s:%d)", __func__, __LINE__); \
        } \
      event_manager_process_event (EVENT_MANAGER, _ev); \
      object_pool_return (EVENT_MANAGER->obj_pool, _ev); \
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
event_manager_start_events (EventManager * self);

/**
 * Stops events from getting fired.
 */
void
event_manager_stop_events (EventManager * self);

/**
 * Processes the given event.
 *
 * The caller is responsible for putting the event
 * back in the object pool if needed.
 */
void
event_manager_process_event (EventManager * self, ZEvent * ev);

/**
 * Processes the events now.
 *
 * Must only be called from the GTK thread.
 */
void
event_manager_process_now (EventManager * self);

/**
 * Removes events where the arg matches the
 * given object.
 */
void
event_manager_remove_events_for_obj (EventManager * self, void * obj);

void
event_manager_free (EventManager * self);

/**
 * @}
 */

#endif
