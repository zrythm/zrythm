// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Timeline selections schema.
 */

#ifndef __SCHEMAS_GUI_BACKEND_TL_SELECTIONS_H__
#define __SCHEMAS_GUI_BACKEND_TL_SELECTIONS_H__

#include "utils/yaml.h"

#include "schemas/dsp/marker.h"
#include "schemas/dsp/region.h"
#include "schemas/dsp/scale_object.h"
#include "schemas/gui/backend/arranger_selections.h"

typedef struct TimelineSelections_v1
{
  ArrangerSelections_v1 base;
  int                   schema_version;
  ZRegion_v1 **         regions;
  int                   num_regions;
  ScaleObject_v1 **     scale_objects;
  int                   num_scale_objects;
  Marker_v1 **          markers;
  int                   num_markers;
  int                   region_track_vis_index;
  int                   chord_track_vis_index;
  int                   marker_track_vis_index;
} TimelineSelections_v1;

static const cyaml_schema_field_t timeline_selections_fields_schema_v1[] = {
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
  YAML_FIELD_INT (TimelineSelections_v1, region_track_vis_index),
  YAML_FIELD_INT (TimelineSelections_v1, chord_track_vis_index),
  YAML_FIELD_INT (TimelineSelections_v1, marker_track_vis_index),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t timeline_selections_schema_v1 = {
  YAML_VALUE_PTR (
    TimelineSelections_v1,
    timeline_selections_fields_schema_v1),
};

TimelineSelections *
timeline_selections_upgrade_from_v1 (
  TimelineSelections_v1 * old);

#endif
