/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Tracklist schema.
 */

#ifndef __SCHEMA_AUDIO_TRACKLIST_H__
#define __SCHEMA_AUDIO_TRACKLIST_H__

typedef struct Tracklist_v1
{
  int        schema_version;
  Track_v1 * tracks[MAX_TRACKS];
  int        num_tracks;
  int        pinned_tracks_cutoff;
} Tracklist_v1;

static const cyaml_schema_field_t
  tracklist_fields_schema_v1[] = {
    YAML_FIELD_INT (Tracklist_v1, schema_version),
    YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
      Tracklist_v1,
      tracks,
      track_schema_v1),
    YAML_FIELD_INT (
      Tracklist_v1,
      pinned_tracks_cutoff),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  tracklist_schema_v1 = {
    YAML_VALUE_PTR (
      Tracklist_v1,
      tracklist_fields_schema_v1),
  };

#endif
