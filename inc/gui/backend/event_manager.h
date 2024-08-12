// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Events for calling refresh on widgets.
 *
 * Note: This is only for refreshing widgets. No
 * logic should be performed here. Any logic must be
 * done before pushing an event.
 */
#ifndef __GUI_BACKEND_EVENT_MANAGER_H__
#define __GUI_BACKEND_EVENT_MANAGER_H__

#include "gui/backend/event.h"
#include "utils/backtrace.h"
#include "utils/mpmc_queue.h"
#include "utils/object_pool.h"

#include <glibmm.h>

/**
 * @addtogroup events
 *
 * @{
 */

constexpr int EVENT_MANAGER_MAX_EVENTS = 4000;

/**
 * Event manager for the UI.
 *
 * This API is responsible for collecting and processing UI
 * events on the GTK thread.
 *
 * @see ZEvent.
 */
class EventManager
{
public:
  /**
   * Creates the event queue and starts the event loop.
   *
   * Must be called from a GTK thread.
   */
  EventManager ();

  ~EventManager () { stop_events (); }

  /**
   * Starts accepting events.
   */
  void start_events ();

  /**
   * Stops events from getting fired.
   */
  void stop_events ();

  /**
   * Processes the given event.
   *
   * The caller is responsible for putting the event back in the object pool if
   * needed.
   */
  void process_event (ZEvent &ev);

  /**
   * @brief Source function to process events.
   */
  bool process_events ();

  /**
   * Processes the events now.
   *
   * Must only be called from the GTK thread.
   */
  void process_now ();

private:
  void clean_duplicate_events_and_copy ();

  /**
   * @brief Source function to recalculate the graph when paused.
   */
  bool soft_recalc_graph_when_paused ();

public:
  /**
   * Event queue, mainly for GUI events.
   */
  MPMCQueue<ZEvent *> mqueue_;

  /**
   * Object pool of event structs to avoid real time
   * allocation.
   */
  ObjectPool<ZEvent> obj_pool_;

  /**
   * ID of the event processing source func.
   *
   * Will be zero when stopped and non-zero when active.
   */
  sigc::scoped_connection process_source_id_;

  /** A soft recalculation of the routing graph is pending. */
  bool pending_soft_recalc_ = false;

private:
  /** Events array to use during processing. */
  std::vector<ZEvent *> events_arr_;
};

#define EVENT_MANAGER (gZrythm->event_manager_)

/** The event queue. */
#define EVENT_QUEUE (EVENT_MANAGER->mqueue_)

/**
 * Push events.
 */
#define EVENTS_PUSH(et, _arg) \
  if ( \
    ZRYTHM_HAVE_UI && (!PROJECT || !AUDIO_ENGINE || !AUDIO_ENGINE->exporting_) \
    && EVENT_MANAGER->process_source_id_.connected ()) \
    { \
      ZEvent * _ev = EVENT_MANAGER->obj_pool_.acquire (); \
      _ev->file_ = __FILE__; \
      _ev->func_ = __func__; \
      _ev->lineno_ = __LINE__; \
      _ev->type_ = (et); \
      _ev->arg_ = (void *) (_arg); \
      if ( \
        zrythm_app->gtk_thread_id == current_thread_id.get () /* skip backtrace \
                                                               for now */ \
        && false) \
        { \
          _ev->backtrace_ = backtrace_get ("", 40, false); \
        } \
      /* don't print events that are called \
       * continuously */ \
      if ( \
        (et) != EventType::ET_PLAYHEAD_POS_CHANGED \
        && zrythm_app->gtk_thread_id == current_thread_id.get ()) \
        { \
          z_debug ("pushing UI event " #et " (%s:%d)", __func__, __LINE__); \
        } \
      EVENT_QUEUE.push_back (_ev); \
    }

/* runs the event logic now */
#define EVENTS_PUSH_NOW(et, _arg) \
  if ( \
    ZRYTHM_HAVE_UI && EVENT_MANAGER \
    && zrythm_app->gtk_thread_id == current_thread_id.get () \
    && (!PROJECT || !AUDIO_ENGINE || !AUDIO_ENGINE->exporting_) \
    && EVENT_MANAGER->process_source_id_.connected ()) \
    { \
      ZEvent * _ev = EVENT_MANAGER->obj_pool_.acquire (); \
      _ev->file_ = __FILE__; \
      _ev->func_ = __func__; \
      _ev->lineno_ = __LINE__; \
      _ev->type_ = et; \
      _ev->arg_ = (void *) _arg; \
      if (/* skip backtrace for now */ \
          false) \
        { \
          _ev->backtrace_ = backtrace_get ("", 40, false); \
        } \
      /* don't print events that are called continuously */ \
      if (et != EventType::ET_PLAYHEAD_POS_CHANGED) \
        { \
          z_debug ( \
            "processing UI event now " #et " (%s:%d)", __func__, __LINE__); \
        } \
      EVENT_MANAGER->process_event (*_ev); \
      EVENT_MANAGER->obj_pool_.release (_ev); \
    }

/**
 * @}
 */

#endif
