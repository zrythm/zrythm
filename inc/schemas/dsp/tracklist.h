// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Tracklist schema.
 */

#ifndef __SCHEMA_AUDIO_TRACKLIST_H__
#define __SCHEMA_AUDIO_TRACKLIST_H__

#include "schemas/dsp/track.h"

typedef struct Tracklist Tracklist;

typedef struct Tracklist_v1
{
  int        schema_version;
  Track_v1 * tracks[MAX_TRACKS];
  int        num_tracks;
  int        pinned_tracks_cutoff;
} Tracklist_v1;

static const cyaml_schema_field_t tracklist_fields_schema_v1[] = {
  YAML_FIELD_INT (Tracklist_v1, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (Tracklist_v1, tracks, track_schema_v1),
  YAML_FIELD_INT (Tracklist_v1, pinned_tracks_cutoff),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t tracklist_schema_v1 = {
  YAML_VALUE_PTR (Tracklist_v1, tracklist_fields_schema_v1),
};

Tracklist *
tracklist_upgrade_from_v1 (Tracklist_v1 * old);

typedef struct Tracklist_v2
{
  int        schema_version;
  Track_v2 * tracks[MAX_TRACKS];
  int        num_tracks;
  int        pinned_tracks_cutoff;
} Tracklist_v2;

static const cyaml_schema_field_t tracklist_fields_schema_v2[] = {
  YAML_FIELD_INT (Tracklist_v2, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (Tracklist_v2, tracks, track_schema_v2),
  YAML_FIELD_INT (Tracklist_v2, pinned_tracks_cutoff),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t tracklist_schema_v2 = {
  YAML_VALUE_PTR (Tracklist_v2, tracklist_fields_schema_v2),
};

Tracklist *
tracklist_upgrade_from_v2 (Tracklist_v2 * old);

#endif
