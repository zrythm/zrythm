// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __SCHEMAS_AUDIO_TRACK_LANE_H__
#define __SCHEMAS_AUDIO_TRACK_LANE_H__

#include "schemas/dsp/region.h"
#include "utils/yaml.h"

typedef struct TrackLane_v1
{
  int           schema_version;
  int           pos;
  char *        name;
  double        height;
  int           mute;
  int           solo;
  ZRegion_v1 ** regions;
  int           num_regions;
  uint8_t       midi_ch;
} TrackLane_v1;

static const cyaml_schema_field_t track_lane_fields_schema_v1[] = {
  YAML_FIELD_INT (TrackLane_v1, schema_version),
  YAML_FIELD_INT (TrackLane_v1, pos),
  YAML_FIELD_STRING_PTR (TrackLane_v1, name),
  YAML_FIELD_FLOAT (TrackLane_v1, height),
  YAML_FIELD_INT (TrackLane_v1, mute),
  YAML_FIELD_INT (TrackLane_v1, solo),
  YAML_FIELD_DYN_ARRAY_VAR_COUNT (TrackLane_v1, regions, region_schema_v1),
  YAML_FIELD_UINT (TrackLane_v1, midi_ch),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t track_lane_schema_v1 = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    TrackLane_v1,
    track_lane_fields_schema_v1),
};

#endif
