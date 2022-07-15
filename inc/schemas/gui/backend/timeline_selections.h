/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Timeline selections schema.
 */

#ifndef __SCHEMAS_GUI_BACKEND_TL_SELECTIONS_H__
#  define __SCHEMAS_GUI_BACKEND_TL_SELECTIONS_H__

#  include "audio/marker.h"
#  include "audio/midi_region.h"
#  include "audio/region.h"
#  include "audio/scale_object.h"
#  include "gui/backend/arranger_selections.h"
#  include "utils/yaml.h"

typedef struct TimelineSelections_v1
{
  ArrangerSelections_v1 base;
  int                   schema_version;
  ZRegion_v1 **         regions;
  int                   num_regions;
  size_t                regions_size;
  ScaleObject_v1 **     scale_objects;
  int                   num_scale_objects;
  size_t                scale_objects_size;
  Marker_v1 **          markers;
  int                   num_markers;
  size_t                markers_size;
  int                   chord_track_vis_index;
  int                   marker_track_vis_index;
} TimelineSelections_v1;

static const cyaml_schema_field_t
  timeline_selections_fields_schema_v1[] = {
    YAML_FIELD_MAPPING_EMBEDDED (
      TimelineSelections_v1,
      base,
      arranger_selections_fields_schema_v1),
    YAML_FIELD_INT (TimelineSelections_v1, schema_version),
    YAML_FIELD_DYN_ARRAY_VAR_COUNT (
      TimelineSelections_v1,
      regions,
      region_schema_v1),
    YAML_FIELD_DYN_ARRAY_VAR_COUNT (
      TimelineSelections_v1,
      scale_objects,
      scale_object_schema_v1),
    YAML_FIELD_DYN_ARRAY_VAR_COUNT (
      TimelineSelections_v1,
      markers,
      marker_schema_v1),
    YAML_FIELD_INT (TimelineSelections_v1, chord_track_vis_index),
    YAML_FIELD_INT (
      TimelineSelections_v1,
      marker_track_vis_index),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t timeline_selections_schema_v1 = {
  YAML_VALUE_PTR (
    TimelineSelections_v1,
    timeline_selections_fields_schema_v1),
};
