/*
 * actions/timeline_selections.h - timeline
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __ACTIONS_TIMELINE_SELECTIONS_H__
#define __ACTIONS_TIMELINE_SELECTIONS_H__

#include "audio/automation_point.h"
#include "audio/chord.h"
#include "audio/midi_region.h"
#include "audio/region.h"
#include "utils/yaml.h"

#define TIMELINE_SELECTIONS (&PROJECT->timeline_selections)

/**
 * Selections to be used for the timeline's current
 * selections, copying, undoing, etc.
 */
typedef struct TimelineSelections
{
  Region *                 regions[600];  ///< regions doing action upon (timeline)
  int                      num_regions;
  Region *                 top_region; ///< highest selected region
  Region *                 bot_region; ///< lowest selected region
  AutomationPoint *        automation_points[600];  ///< automation points doing action upon (timeline)
  int                      num_automation_points;
  Chord *                  chords[800];
  int                      num_chords;
} TimelineSelections;

static const cyaml_schema_field_t
  timeline_selections_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    "regions", CYAML_FLAG_OPTIONAL,
      TimelineSelections, regions, num_regions,
      &region_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_MAPPING_PTR (
    "top_region", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER,
    TimelineSelections, top_region, region_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "bot_region", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    TimelineSelections, bot_region, region_fields_schema),
  CYAML_FIELD_SEQUENCE_COUNT (
    "automation_points", CYAML_FLAG_OPTIONAL,
      TimelineSelections, automation_points, num_automation_points,
      &automation_point_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "chords", CYAML_FLAG_OPTIONAL,
    TimelineSelections, chords, num_chords,
    &chord_schema, 0, CYAML_UNLIMITED),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
timeline_selections_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
			TimelineSelections, timeline_selections_fields_schema),
};
/**
 * Clone the struct for copying, undoing, etc.
 */
//void
//timeline_selections_clone (TimelineSelections * src,
                           //TimelineSelections * dest);

/**
 * Returns the position of the leftmost object.
 */
void
timeline_selections_get_start_pos (
  TimelineSelections * ts,
  Position *                pos); ///< position to fill in

void
timeline_selections_paste_to_pos (
  TimelineSelections * ts,
  Position *           pos);

SERIALIZE_INC (TimelineSelections, timeline_selections)
DESERIALIZE_INC (TimelineSelections, timeline_selections)
PRINT_YAML_INC (TimelineSelections, timeline_selections)

#endif
