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

#include "inc/utils/stack.h"

#define EVENTS_PUSH(et, arg) \
  if (EVENTS->et_stack && EVENTS->arg_stack) \
    { \
      STACK_PUSH (EVENTS->et_stack, et); \
      STACK_PUSH (EVENTS->arg_stack, arg); \
    }
#define EVENTS (&ZRYTHM->events)

typedef enum EventType
{
  ET_REGION_REMOVED,
  /** also for channels */
  ET_TRACK_STATE_CHANGED,
  ET_REGION_CREATED,
  ET_CHORD_CREATED,
  ET_CHORD_REMOVED,
  /** works for all rulers */
  ET_RULER_STATE_CHANGED,
  ET_AUTOMATION_LANE_ADDED,
  ET_TIME_SIGNATURE_CHANGED,
  ET_TRACK_ADDED,

  ET_LAST_TIMELINE_OBJECT_CHANGED,

  ET_TRACK_SELECT_CHANGED,
  ET_TRACK_BOT_PANED_VISIBILITY_CHANGED,
  ET_AUTOMATION_LANE_AUTOMATION_TRACK_CHANGED,

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

  /** Selected tool (mode) changed. */
  ET_TOOL_CHANGED,

  /**
   * Zoom level or view area changed.
   */
  ET_TIMELINE_VIEWPORT_CHANGED,
  ET_PLUGIN_ADDED,
  ET_PLAYHEAD_POS_CHANGED,

  /** MidiNote changed. */
  ET_MIDI_NOTE_CHANGED,
  ET_TL_SELECTIONS_CHANGED,
  ET_MIDI_ARRANGER_SELECTIONS_CHANGED,
  ET_AUTOMATION_VALUE_CHANGED,

  /** Plugin visibility changed, should close/open
   * UI. */
  ET_PLUGIN_VISIBILITY_CHANGED,
} EventType;

typedef struct Events
{
  /** Event types stack.
   *
   * Contains EventTypes. */
  Stack *       et_stack;

  /** Arguments stack.
   *
   * This contains the arguments passed to the event */
  Stack *       arg_stack;
} Events;

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
void
events_init (Events * events);

#endif
