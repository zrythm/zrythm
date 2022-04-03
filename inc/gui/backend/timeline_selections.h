/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * @addtogroup gui_backend
 *
 * @{
 */

#define TL_SELECTIONS_SCHEMA_VERSION 1

#define TL_SELECTIONS (PROJECT->timeline_selections)

/**
 * Selections to be used for the timeline's current
 * selections, copying, undoing, etc.
 */
typedef struct TimelineSelections
{
  /** Base struct. */
  ArrangerSelections base;

  int schema_version;

  /** Selected TrackLane Region's. */
  ZRegion ** regions;
  int        num_regions;
  size_t     regions_size;

  ScaleObject ** scale_objects;
  int            num_scale_objects;
  size_t         scale_objects_size;

  Marker ** markers;
  int       num_markers;
  size_t    markers_size;

  /** Visible track index, used during copying. */
  int region_track_vis_index;

  /** Visible track index, used during copying. */
  int chord_track_vis_index;

  /** Visible track index, used during copying. */
  int marker_track_vis_index;
} TimelineSelections;

static const cyaml_schema_field_t
  timeline_selections_fields_schema[] = {
    YAML_FIELD_MAPPING_EMBEDDED (
      TimelineSelections,
      base,
      arranger_selections_fields_schema),
    YAML_FIELD_INT (TimelineSelections, schema_version),
    YAML_FIELD_DYN_ARRAY_VAR_COUNT (
      TimelineSelections,
      regions,
      region_schema),
    YAML_FIELD_DYN_ARRAY_VAR_COUNT (
      TimelineSelections,
      scale_objects,
      scale_object_schema),
    YAML_FIELD_DYN_ARRAY_VAR_COUNT (
      TimelineSelections,
      markers,
      marker_schema),
    YAML_FIELD_INT (
      TimelineSelections,
      region_track_vis_index),
    YAML_FIELD_INT (
      TimelineSelections,
      chord_track_vis_index),
    YAML_FIELD_INT (
      TimelineSelections,
      marker_track_vis_index),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  timeline_selections_schema = {
    YAML_VALUE_PTR (
      TimelineSelections,
      timeline_selections_fields_schema),
  };

/**
 * Creates a new TimelineSelections instance for
 * the given range.
 *
 * @bool clone_objs True to clone each object,
 *   false to use pointers to project objects.
 */
TimelineSelections *
timeline_selections_new_for_range (
  Position * start_pos,
  Position * end_pos,
  bool       clone_objs);

/**
 * Gets highest track in the selections.
 */
Track *
timeline_selections_get_first_track (
  TimelineSelections * ts);

/**
 * Gets lowest track in the selections.
 */
Track *
timeline_selections_get_last_track (
  TimelineSelections * ts);

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

/**
 * @param with_parents Also mark all the track's
 *   parents recursively.
 */
void
timeline_selections_mark_for_bounce (
  TimelineSelections * ts,
  bool                 with_parents);

/**
 * Move the selected Regions to new Lanes.
 *
 * @param diff The delta to move the
 *   Tracks.
 *
 * @return True if moved.
 */
bool
timeline_selections_move_regions_to_new_lanes (
  TimelineSelections * self,
  const int            diff);

/**
 * Move the selected Regions to the new Track.
 *
 * @param new_track_is_before 1 if the Region's
 *   should move to their previous tracks, 0 for
 *   their next tracks.
 *
 * @return True if moved.
 */
bool
timeline_selections_move_regions_to_new_tracks (
  TimelineSelections * self,
  const int            vis_track_diff);

/**
 * Sets the regions'
 * \ref ZRegion.index_in_prev_lane.
 */
void
timeline_selections_set_index_in_prev_lane (
  TimelineSelections * self);

NONNULL
bool
timeline_selections_contains_only_regions (
  const TimelineSelections * self);

NONNULL
bool
timeline_selections_contains_only_region_types (
  const TimelineSelections * self,
  RegionType                 types);

/**
 * Exports the selections to the given MIDI file.
 */
NONNULL
bool
timeline_selections_export_to_midi_file (
  const TimelineSelections * self,
  const char *               full_path,
  int                        midi_version,
  const bool                 export_full_regions,
  const bool                 lanes_as_tracks);

#define timeline_selections_move_w_action( \
  sel, ticks, delta_tracks, delta_lanes, \
  already_moved) \
  arranger_selections_move_w_action ( \
    sel, ticks, 0, 0, delta_tracks, delta_lanes, \
    0, already_moved)

#define timeline_selections_duplicate_w_action( \
  sel, ticks, delta_tracks, delta_lanes, \
  already_moved) \
  arranger_selections_duplicate_w_action ( \
    sel, ticks, 0, 0, delta_tracks, delta_lanes, \
    0, already_moved)

/**
 * @}
 */

#endif
