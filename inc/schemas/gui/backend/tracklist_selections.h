// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Tracklist selections schema.
 */

#ifndef __SCHEMAS_ACTIONS_TRACKLIST_SELECTIONS_H__
#define __SCHEMAS_ACTIONS_TRACKLIST_SELECTIONS_H__

#include "utils/yaml.h"

#include "schemas/dsp/track.h"

typedef struct TracklistSelections TracklistSelections;

typedef struct TracklistSelections_v1
{
  int        schema_version;
  Track_v1 * tracks[600];
  int        num_tracks;
  bool       is_project;
} TracklistSelections_v1;

static const cyaml_schema_field_t tracklist_selections_fields_schema_v1[] = {
  YAML_FIELD_INT (TracklistSelections_v1, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    TracklistSelections_v1,
    tracks,
    track_schema_v1),
  YAML_FIELD_INT (TracklistSelections_v1, is_project),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t tracklist_selections_schema_v1 = {
  YAML_VALUE_PTR (TracklistSelections_v1, tracklist_selections_fields_schema_v1),
};

TracklistSelections *
tracklist_selections_upgrade_from_v1 (TracklistSelections_v1 * old);

#endif
