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
 * Action for quantizing the timeline selections.
 */

#ifndef __UNDO_QUANTIZE_TIMELINE_SELECTIONS_ACTION_H__
#define __UNDO_QUANTIZE_TIMELINE_SELECTIONS_ACTION_H__

#include "actions/undoable_action.h"
#include "audio/position.h"

typedef struct TimelineSelections
  TimelineSelections;
typedef struct QuantizeOptions QuantizeOptions;

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * Action to quantize the given TimelineSelections
 * using the given QuantizeOptions.
 */
typedef struct QuantizeTimelineSelectionsAction
{
  UndoableAction       parent_instance;

  /** Timeline selections clone with original
   * positions. */
  TimelineSelections * unquantized_ts;

  /** Timeline selections clone with quantized
   * positions. */
  TimelineSelections * quantized_ts;

  /** QuantizeOptions clone. */
  QuantizeOptions *    opts;

} QuantizeTimelineSelectionsAction;

/**
 * Creates a new action.
 */
UndoableAction *
quantize_timeline_selections_action_new (
  const TimelineSelections * ts,
  const QuantizeOptions *    opts);

int
quantize_timeline_selections_action_do (
	QuantizeTimelineSelectionsAction * self);

int
quantize_timeline_selections_action_undo (
	QuantizeTimelineSelectionsAction * self);

char *
quantize_timeline_selections_action_stringize (
	QuantizeTimelineSelectionsAction * self);

void
quantize_timeline_selections_action_free (
	QuantizeTimelineSelectionsAction * self);

/**
 * @}
 */

#endif
