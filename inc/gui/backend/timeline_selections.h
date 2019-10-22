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

#include "audio/marker.h"
#include "audio/midi_region.h"
#include "audio/region.h"
#include "audio/scale_object.h"
#include "gui/backend/arranger_selections.h"
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

  /** Selected ScaleObject's. */
  ScaleObject *       scale_objects[800];
  int                 num_scale_objects;

  /** Selected Marker's. */
  Marker *            markers[200];
  int                 num_markers;

  /** Visible track index, used during copying. */
  int                 chord_track_vis_index;

  /** Visible track index, used during copying. */
  int                 marker_track_vis_index;
} TimelineSelections;

static const cyaml_schema_field_t
  timeline_selections_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    "regions", CYAML_FLAG_DEFAULT,
    TimelineSelections, regions, num_regions,
    &region_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "scale_objects", CYAML_FLAG_DEFAULT,
    TimelineSelections, scale_objects,
    num_scale_objects,
    &scale_object_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "markers", CYAML_FLAG_DEFAULT,
    TimelineSelections, markers, num_markers,
    &marker_schema, 0, CYAML_UNLIMITED),
	CYAML_FIELD_INT (
    "chord_track_vis_index", CYAML_FLAG_DEFAULT,
    TimelineSelections, chord_track_vis_index),
	CYAML_FIELD_INT (
    "marker_track_vis_index", CYAML_FLAG_DEFAULT,
    TimelineSelections, marker_track_vis_index),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
timeline_selections_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER, TimelineSelections,
    timeline_selections_fields_schema),
};

ARRANGER_SELECTIONS_DECLARE_TIMELINE_FUNCS (
  Timeline, timeline);
ARRANGER_SELECTIONS_DECLARE_OBJ_FUNCS (
  Timeline, timeline, Region, region);
ARRANGER_SELECTIONS_DECLARE_OBJ_FUNCS (
  Timeline, timeline, ScaleObject, scale_object);
ARRANGER_SELECTIONS_DECLARE_OBJ_FUNCS (
  Timeline, timeline, Marker, marker);

/**
 * Gets highest track in the selections.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
Track *
timeline_selections_get_first_track (
  TimelineSelections * ts,
  int                  transient);

/**
 * Gets lowest track in the selections.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
Track *
timeline_selections_get_last_track (
  TimelineSelections * ts,
  int                  transient);

/**
 * Replaces the track positions in each object with
 * visible track indices starting from 0.
 *
 * Used during copying.
 */
void
timeline_selections_set_vis_track_indices (
  TimelineSelections * ts);

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param idx Track index to start pasting to.
 */
int
timeline_selections_can_be_pasted (
  TimelineSelections * ts,
  Position *           pos,
  const int            idx);

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
