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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Current TimelineArranger selections.
 */

#ifndef __GUI_BACKEND_TL_SELECTIONS_H__
#define __GUI_BACKEND_TL_SELECTIONS_H__

#include "audio/automation_point.h"
#include "audio/chord_object.h"
#include "audio/marker.h"
#include "audio/midi_region.h"
#include "audio/region.h"
#include "audio/scale_object.h"
#include "utils/yaml.h"

/**
 * @addtogroup gui_backend GUI backend
 *
 * @{
 */

#define TL_SELECTIONS \
  (&PROJECT->timeline_selections)

/**
 * Selections to be used for the timeline's current
 * selections, copying, undoing, etc.
 */
typedef struct TimelineSelections
{
  /** Selected TrackLane Region's. */
  Region *            regions[600];
  int                 num_regions;

  /** Selected AutomationPoint's. */
  AutomationPoint *   automation_points[600];
  int                 num_automation_points;

  /** Selected ChordObject's. */
  ChordObject *       chord_objects[800];
  int                 num_chord_objects;

  /** Selected ScaleObject's. */
  ScaleObject *       scale_objects[800];
  int                 num_scale_objects;

  /** Selected Marker's. */
  Marker *            markers[200];
  int                 num_markers;
} TimelineSelections;

static const cyaml_schema_field_t
  timeline_selections_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    "regions", CYAML_FLAG_DEFAULT,
    TimelineSelections, regions, num_regions,
    &region_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aps", CYAML_FLAG_DEFAULT,
    TimelineSelections, automation_points,
    num_automation_points,
    &automation_point_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "chords", CYAML_FLAG_DEFAULT,
    TimelineSelections, chord_objects,
    num_chord_objects,
    &chord_object_schema, 0, CYAML_UNLIMITED),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
timeline_selections_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER, TimelineSelections,
    timeline_selections_fields_schema),
};

void
timeline_selections_init_loaded (
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
 * Returns if the timeline object is in the
 * selections or  not.
 */
#define CONTAINS_FUNC_DECL(cc,sc) \
  int \
  timeline_selections_contains_##sc ( \
    TimelineSelections * self, \
    cc *                 sc)

CONTAINS_FUNC_DECL (Region, region);
CONTAINS_FUNC_DECL (ChordObject, chord_object);
CONTAINS_FUNC_DECL (ScaleObject, scale_object);
CONTAINS_FUNC_DECL (Marker, marker);
CONTAINS_FUNC_DECL (
  AutomationPoint, automation_point);

#undef CONTAINS_FUNC_DECL

/**
 * Adds the timeline object to the selections.
 */
#define ADD_FUNC_DECL(cc,sc) \
  void \
  timeline_selections_add_##sc ( \
    TimelineSelections * ts, \
    cc *                 sc)

ADD_FUNC_DECL (Region, region);
ADD_FUNC_DECL (ChordObject, chord_object);
ADD_FUNC_DECL (ScaleObject, scale_object);
ADD_FUNC_DECL (Marker, marker);
ADD_FUNC_DECL (
  AutomationPoint, automation_point);

#undef ADD_FUNC_DECL

/**
 * Removes a timeline object from the selections.
 */
#define REMOVE_FUNC_DECL(cc,sc) \
  void \
  timeline_selections_remove_##sc ( \
    TimelineSelections * ts, \
    cc *                 sc)

REMOVE_FUNC_DECL (Region, region);
REMOVE_FUNC_DECL (ChordObject, chord_object);
REMOVE_FUNC_DECL (ScaleObject, scale_object);
REMOVE_FUNC_DECL (Marker, marker);
REMOVE_FUNC_DECL (
  AutomationPoint, automation_point);

#undef REMOVE_FUNC_DECL

/**
 * Sets the cache Position's for each object in
 * the selection.
 *
 * Used by the ArrangerWidget's.
 */
void
timeline_selections_set_cache_poses (
  TimelineSelections * ts);

/**
 * Set all transient Position's to their main
 * counterparts.
 */
void
timeline_selections_reset_transient_poses (
  TimelineSelections * ts);

/**
 * Set all main Position's to their transient
 * counterparts.
 */
void
timeline_selections_set_to_transient_poses (
  TimelineSelections * ts);

/**
 * Similar to set_to_transient_poses, but handles
 * values for objects that support them (like
 * AutomationPoint's).
 */
void
timeline_selections_set_to_transient_values (
  TimelineSelections * ts);

/**
 * Moves the TimelineSelections by the given
 * amount of ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position's instead of the current Position's.
 * @param ticks Ticks to add.
 * @param transients_only Only update transient
 *   objects (eg. when copy-moving).
 */
void
timeline_selections_add_ticks (
  TimelineSelections * ts,
  long                 ticks,
  int                  use_cached_pos,
  int                  transients_only);

/**
 * Clears selections.
 */
void
timeline_selections_clear (
  TimelineSelections * ts);

void
timeline_selections_free (TimelineSelections * self);

SERIALIZE_INC (
  TimelineSelections, timeline_selections)
DESERIALIZE_INC (
  TimelineSelections, timeline_selections)
PRINT_YAML_INC (
  TimelineSelections, timeline_selections)

/**
 * @}
 */

#endif
