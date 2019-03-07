/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __AUDIO_AUTOMATION_LANE_H__
#define __AUDIO_AUTOMATION_LANE_H__

#include "audio/position.h"
#include "audio/automation_curve.h"
#include "audio/automation_point.h"
#include "gui/widgets/automation_lane.h"

/**
 * Automation lanes have a one-on-one relationship
 * with automation tracks.
 *
 * Automation lanes represent what is shown on the UI
 * and are associated with an automation track for
 * which the lane displays automation information.
 */
typedef struct AutomationLane
{
  int                     id;

  /**
   * The automation track this automation lane is for.
   */
  int                     at_id;
  AutomationTrack *       at; ///< cache

  /** Whether visible or not. */
  int                     visible;

  /**
   * Position of multipane handle.
   */
  int                     handle_pos;

  /** Widget. */
  AutomationLaneWidget *  widget;
} AutomationLane;

static const cyaml_schema_field_t
  automation_lane_fields_schema[] =
{
	CYAML_FIELD_INT (
    "id", CYAML_FLAG_DEFAULT,
    AutomationLane, id),
	CYAML_FIELD_INT (
    "at_id", CYAML_FLAG_DEFAULT,
    AutomationLane, at_id),
	CYAML_FIELD_INT (
    "visible", CYAML_FLAG_DEFAULT,
    AutomationLane, visible),
	CYAML_FIELD_INT (
    "handle_pos", CYAML_FLAG_DEFAULT,
    AutomationLane, handle_pos),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
  automation_lane_schema =
{
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    AutomationLane,
    automation_lane_fields_schema),
};

void
automation_lane_init_loaded (
  AutomationLane * self);

/**
 * Creates an automation lane for the given
 * automation track.
 */
AutomationLane *
automation_lane_new (
  AutomationTrack * automation_track);

/**
 * Updates the automation track in this lane and
 * updates the UI to reflect the change.
 *
 * TODO
 */
void
automation_lane_update_automation_track (
  AutomationLane * self,
  AutomationTrack * at);

void
automation_lane_free (AutomationLane * self);

#endif // __AUDIO_AUTOMATION_LANE_H__
