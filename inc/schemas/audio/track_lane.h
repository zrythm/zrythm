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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Track lane schema.
 */

#ifndef __SCHEMAS_AUDIO_TRACK_LANE_H__
#define __SCHEMAS_AUDIO_TRACK_LANE_H__

#include "audio/region.h"
#include "utils/yaml.h"

typedef struct TrackLane_v1
{
  int           schema_version;
  int           pos;
  char *        name;
  void *        widget;
  int           y;
  double        height;
  int           mute;
  int           solo;
  ZRegion_v1 ** regions;
  int           num_regions;
  size_t        regions_size;
  int           track_pos;
  uint8_t       midi_ch;
  void *        buttons[8];
  int           num_buttons;
} TrackLane_v1;

static const cyaml_schema_field_t
  track_lane_fields_schema_v1[] = {
    YAML_FIELD_INT (TrackLane_v1, schema_version),
    YAML_FIELD_INT (TrackLane_v1, pos),
    YAML_FIELD_STRING_PTR (TrackLane_v1, name),
    YAML_FIELD_FLOAT (TrackLane_v1, height),
    YAML_FIELD_INT (TrackLane_v1, mute),
    YAML_FIELD_INT (TrackLane_v1, solo),
    YAML_FIELD_DYN_ARRAY_VAR_COUNT (
      TrackLane_v1,
      regions,
      region_schema),
    YAML_FIELD_INT (TrackLane_v1, track_pos),
    YAML_FIELD_UINT (TrackLane_v1, midi_ch),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  track_lane_schema_v1 = {
    CYAML_VALUE_MAPPING (
      CYAML_FLAG_POINTER,
      TrackLane_v1,
      track_lane_fields_schema_v1),
  };

#endif
