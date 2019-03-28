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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __ACTIONS_TL_SELECTIONS_H__
#define __ACTIONS_TL_SELECTIONS_H__

#include "audio/automation_point.h"
#include "audio/chord.h"
#include "audio/midi_region.h"
#include "audio/region.h"
#include "utils/yaml.h"

#define TL_SELECTIONS (&PROJECT->timeline_selections)

/**
 * Selections to be used for the timeline's current
 * selections, copying, undoing, etc.
 */
typedef struct TimelineSelections
{
  /** Regions doing action upon */
  Region *                 regions[600];
  Region *                 transient_regions[600];
  int                      region_ids[600];
  int                      num_regions;

  /** Highest selected region */
  //Region *                 top_region;
  //int                      top_region_id;

  /** Lowest selected region */
  //Region *                 bot_region;
  //int                      bot_region_id;

  /** Automation points acting upon */
  AutomationPoint *        automation_points[600];
  AutomationPoint *        transient_aps[600];
  int                      ap_ids[600];
  int                      num_automation_points;

  /** Chords acting upon */
  Chord *                  chords[800];
  Chord *                  transient_chords[800];
  int                      chord_ids[800];
  int                      num_chords;
} TimelineSelections;

static const cyaml_schema_field_t
  timeline_selections_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    "region_ids", CYAML_FLAG_DEFAULT,
    TimelineSelections, region_ids,
    num_regions,
    &int_schema, 0, CYAML_UNLIMITED),
	//CYAML_FIELD_INT (
    //"top_region_id", CYAML_FLAG_DEFAULT,
    //TimelineSelections, top_region_id),
	//CYAML_FIELD_INT (
    //"bot_region_id", CYAML_FLAG_DEFAULT,
    //TimelineSelections, bot_region_id),
  CYAML_FIELD_SEQUENCE_COUNT (
    "ap_ids", CYAML_FLAG_DEFAULT,
    TimelineSelections, ap_ids,
    num_automation_points,
    &int_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "chord_ids", CYAML_FLAG_DEFAULT,
    TimelineSelections, chord_ids,
    num_chords,
    &int_schema, 0, CYAML_UNLIMITED),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
timeline_selections_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
			TimelineSelections, timeline_selections_fields_schema),
};

void
timeline_selections_init_loaded (
  TimelineSelections * ts);

/**
 * Creates transient objects for objects added
 * to selections without transients.
 */
void
timeline_selections_create_missing_transients (
  TimelineSelections * ts);

/**
 * Clone the struct for copying, undoing, etc.
 */
TimelineSelections *
timeline_selections_clone ();

/**
 * Returns if there are any selections.
 */
int
timeline_selections_has_any (
  TimelineSelections * ts);

/**
 * Returns the position of the leftmost object.
 *
 * If transient is 1, the transient objects are
 * checked instead.
 *
 * The return value will be stored in pos.
 */
void
timeline_selections_get_start_pos (
  TimelineSelections * ts,
  Position *           pos,
  int                  transient);

/**
 * Returns the position of the rightmost object.
 *
 * If transient is 1, the transient objects are
 * checked instead.
 *
 * The return value will be stored in pos.
 */
void
timeline_selections_get_end_pos (
  TimelineSelections * ts,
  Position *           pos,
  int                  transient);

/**
 * Gets first object's widget.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
GtkWidget *
timeline_selections_get_first_object (
  TimelineSelections * ts,
  int                  transient);

/**
 * Gets last object's widget.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
GtkWidget *
timeline_selections_get_last_object (
  TimelineSelections * ts,
  int                  transient);

/**
 * Gets highest track in the selections.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
Track *
timeline_selections_get_highest_track (
  TimelineSelections * ts,
  int                  transient);

/**
 * Gets lowest track in the selections.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
Track *
timeline_selections_get_lowest_track (
  TimelineSelections * ts,
  int                  transient);

void
timeline_selections_paste_to_pos (
  TimelineSelections * ts,
  Position *           pos);

/**
 * Only removes transients from their tracks and
 * frees them.
 */
void
timeline_selections_remove_transients (
  TimelineSelections * ts);

/**
 * Adds an object to the selections.
 *
 * Optionally adds a transient object (if moving/
 * copy-moving).
 */
//void
//timeline_selections_add_object (
  //TimelineSelections * ts,
  //GtkWidget *          object,
  //int                  transient);

void
timeline_selections_add_region (
  TimelineSelections * ts,
  Region *             r,
  int                  transient);

void
timeline_selections_add_chord (
  TimelineSelections * ts,
  Chord *              c,
  int                  transient);

void
timeline_selections_add_ap (
  TimelineSelections * ts,
  AutomationPoint *    ap,
  int                  transient);

//void
//timeline_selections_remove_object (
  //TimelineSelections * ts,
  //GtkWidget *          object);

void
timeline_selections_remove_region (
  TimelineSelections * ts,
  Region *             r);

void
timeline_selections_remove_chord (
  TimelineSelections * ts,
  Chord *              c);

void
timeline_selections_remove_ap (
  TimelineSelections * ts,
  AutomationPoint *    ap);

/**
 * Clears selections.
 */
void
timeline_selections_clear (
  TimelineSelections * ts);

void
timeline_selections_free (TimelineSelections * self);

SERIALIZE_INC (TimelineSelections, timeline_selections)
DESERIALIZE_INC (TimelineSelections, timeline_selections)
PRINT_YAML_INC (TimelineSelections, timeline_selections)

#endif
