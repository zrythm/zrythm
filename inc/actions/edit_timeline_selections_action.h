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
 * Action for editing objects in the timeline.
 */

#ifndef __UNDO_EDIT_TIMELINE_SELECTIONS_ACTION_H__
#define __UNDO_EDIT_TIMELINE_SELECTIONS_ACTION_H__

#include "actions/undoable_action.h"
#include "audio/position.h"

typedef struct TimelineSelections
  TimelineSelections;

/**
 * @addtogroup actions
 *
 * @{
 */

typedef enum EditTimelineSelectionsType
{
  /** Resize one or more objects. */
  ETS_RESIZE_L,
  /** Resize one or more object. */
  ETS_RESIZE_R,
  /** Edit name for a single Region. */
  ETS_REGION_NAME,
  ETS_REGION_CLIP_START_POS,
  ETS_REGION_LOOP_START_POS,
  ETS_REGION_LOOP_END_POS,
  /** Edit parameters for a single ChordObject. */
  ETS_CHORD_OBJECT,
  /** Edit parameters for a single ScaleObject. */
  ETS_SCALE_OBJECT,
  /** Edit parameters for a single Marker. */
  ETS_MARKER,
  /** Cut an object (usually Region). */
  ETS_CUT,
} EditTimelineSelectionsType;

typedef struct EditTimelineSelectionsAction
{
  UndoableAction              parent_instance;

  /** Timeline selections clone. */
  TimelineSelections * ts;

  /** Type of action. */
  EditTimelineSelectionsType type;

  /** String, when changing a string. */
  char *               str;

  /** Position, when changing a Position. */
  Position             pos;

  /**
   * If this is 1, the first "do" call does
   * nothing in some cases.
   *
   * Set internally and either used or ignored.
   */
  int                  first_call;

  /** Amount to resize in ticks (negative for
   * backwards), if resizing. */
  long                 ticks;
} EditTimelineSelectionsAction;

UndoableAction *
edit_timeline_selections_action_new (
  TimelineSelections * ts,
  EditTimelineSelectionsType type,
  const long       ticks,
  const char *     str,
  const Position * pos);

int
edit_timeline_selections_action_do (
	EditTimelineSelectionsAction * self);

int
edit_timeline_selections_action_undo (
	EditTimelineSelectionsAction * self);

char *
edit_timeline_selections_action_stringize (
	EditTimelineSelectionsAction * self);

void
edit_timeline_selections_action_free (
	EditTimelineSelectionsAction * self);

/**
 * @}
 */

#endif
