/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
#ifndef __GUI_BACKEND_EVENTS_H__
#define __GUI_BACKEND_EVENTS_H__

#include "zix/sem.h"
#include "utils/stack.h"

/**
 * @addtogroup events
 *
 * @{
 */

/**
 * Push events.
 *
 * Note: not real-time safe.
 */
#define EVENTS_PUSH(et,_arg) \
  if (EVENTS && \
      !AUDIO_ENGINE->exporting) \
    { \
      ZEvent * ev = calloc (1, sizeof (ZEvent)); \
      ev->type = et; \
      ev->arg = _arg; \
      g_async_queue_push (EVENTS, ev); \
    }

/** The event queue. */
#define EVENTS (ZRYTHM->event_queue)

typedef enum EventType
{
  ET_REGION_REMOVED,
  ET_CHORD_OBJECT_REMOVED,
  ET_SCALE_OBJECT_REMOVED,
  ET_MARKER_REMOVED,
  /** also for channels */
  ET_TRACK_STATE_CHANGED,
  ET_REGION_CREATED,
  ET_CHORD_OBJECT_CREATED,
  ET_SCALE_OBJECT_CREATED,
  ET_MARKER_CREATED,
  /** works for all rulers */
  ET_RULER_STATE_CHANGED,
  ET_AUTOMATION_TRACK_ADDED,
  ET_TIME_SIGNATURE_CHANGED,
  ET_TRACK_ADDED,
  ET_TRACK_CHANGED,
  ET_TRACK_COLOR_CHANGED,
  ET_TRACK_NAME_CHANGED,

  ET_LAST_TIMELINE_OBJECT_CHANGED,

  ET_TRACK_BOT_PANED_VISIBILITY_CHANGED,
  ET_AUTOMATION_TRACK_CHANGED,

  /**
   * Region (clip) to show in the piano roll
   * changed.
   *
   * Eg., a region in the timeline was clicked.
   */
  ET_CLIP_EDITOR_REGION_CHANGED,

  /**
   * Clip marker (clip start, loop start, loop end)
   * position changed.
   */
  ET_CLIP_MARKER_POS_CHANGED,

  ET_UNDO_REDO_ACTION_DONE,
  ET_RANGE_SELECTION_CHANGED,
  ET_TIMELINE_LOOP_MARKER_POS_CHANGED,
  ET_TIMELINE_SONG_MARKER_POS_CHANGED,
  ET_RULER_SIZE_CHANGED,
  ET_MIDI_NOTE_CREATED,
  ET_MIDI_NOTE_REMOVED,

  ET_REGION_CHANGED,
  ET_AUTOMATION_POINT_CHANGED,
  ET_AUTOMATION_POINT_REMOVED,
  ET_AUTOMATION_CURVE_REMOVED,
  ET_CHORD_OBJECT_CHANGED,
  ET_SCALE_OBJECT_CHANGED,
  ET_MARKER_CHANGED,
  ET_ARRANGER_OBJECT_SELECTION_CHANGED,

  ET_LOOP_TOGGLED,

  /** Selected tool (mode) changed. */
  ET_TOOL_CHANGED,

  /**
   * Zoom level or view area changed.
   */
  ET_TIMELINE_VIEWPORT_CHANGED,
  ET_PLUGIN_ADDED,
  ET_PLUGINS_ADDED,
  ET_PLUGINS_REMOVED,
  ET_PLUGIN_DELETED,
  ET_PLAYHEAD_POS_CHANGED,

  /** MidiNote changed. */
  ET_MIDI_NOTE_CHANGED,
  ET_TL_SELECTIONS_CHANGED,
  ET_TL_SELECTIONS_CREATED,
  ET_TL_SELECTIONS_REMOVED,
  ET_MA_SELECTIONS_CHANGED,
  ET_TRACKLIST_SELECTIONS_CHANGED,
  ET_AUTOMATION_VALUE_CHANGED,

  /** Plugin visibility changed, should close/open
   * UI. */
  ET_PLUGIN_VISIBILITY_CHANGED,
  ET_PLUGIN_SELECTION_CHANGED,

  ET_TRACKS_ADDED,
  ET_TRACKS_REMOVED,
  ET_TRACKS_MOVED,
  ET_CHANNEL_REMOVED,
  ET_REFRESH_ARRANGER,
  ET_MIXER_SELECTIONS_CHANGED,
  ET_CHANNEL_OUTPUT_CHANGED,
  ET_CHANNEL_SLOTS_CHANGED,
  ET_DRUM_MODE_CHANGED,
  ET_MODULATOR_ADDED,
  ET_RT_SELECTIONS_CHANGED,
  ET_PINNED_TRACKLIST_SIZE_CHANGED,
  ET_TRACK_LANES_VISIBILITY_CHANGED,
  ET_AP_CHANGED,
  ET_AUTOMATION_POINT_CREATED,
  ET_AUTOMATION_CURVE_CREATED,
  ET_TRACK_LANE_ADDED,
  ET_REGION_POSITIONS_CHANGED,
  ET_CHORD_OBJECT_POSITIONS_CHANGED,
  ET_SCALE_OBJECT_POSITIONS_CHANGED,
  ET_MARKER_POSITIONS_CHANGED,
  ET_AUTOMATION_POINT_POSITIONS_CHANGED,
  ET_PIANO_ROLL_HIGHLIGHTING_CHANGED,
  ET_PIANO_ROLL_MIDI_MODIFIER_CHANGED,
  ET_TIMELINE_OBJECTS_IN_TRANSIT,
  ET_AUTOMATION_TRACKLIST_AT_REMOVED,
  ET_CHORD_OBJECTS_IN_TRANSIT,
  ET_CHORD_SELECTIONS_CHANGED,
  ET_AUTOMATION_OBJECTS_IN_TRANSIT,
  ET_AUTOMATION_SELECTIONS_CHANGED,
} EventType;

/**
 * A Zrythm event.
 */
typedef struct ZEvent
{
  EventType     type;
  void *        arg;
} ZEvent;

/**
 * GSourceFunc to be added using idle add.
 *
 * This will loop indefinintely.
 */
int
events_process ();

/**
 * Must be called from a GTK thread.
 */
GAsyncQueue *
events_init ();

/**
 * @}
 */

#endif
