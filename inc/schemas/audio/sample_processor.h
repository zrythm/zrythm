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
 * Sample processor schema.
 */

#ifndef __SCHEMAS_AUDIO_SAMPLE_PROCESSOR_H__
#define __SCHEMAS_AUDIO_SAMPLE_PROCESSOR_H__

#include "audio/sample_playback.h"
#include "utils/types.h"

#include "schemas/audio/port.h"

typedef struct SamplePlayback_v1
{
  sample_t ** buf;
  channels_t  channels;
  long        buf_size;
  long        offset;
  float       volume;
  nframes_t   start_offset;
} SamplePlayback_v1;

typedef struct SampleProcessor_v1
{
  int               schema_version;
  SamplePlayback_v1 current_samples[256];
  int               num_current_samples;
  StereoPorts_v1 *  stereo_out;
} SampleProcessor_v1;

static const cyaml_schema_field_t
  sample_processor_fields_schema_v1[] = {
    YAML_FIELD_INT (
      SampleProcessor_v1,
      schema_version),
    YAML_FIELD_MAPPING_PTR (
      SampleProcessor_v1,
      stereo_out,
      stereo_ports_fields_schema_v1),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  sample_processor_schema_v1 = {
    YAML_VALUE_PTR (
      SampleProcessor_v1,
      sample_processor_fields_schema_v1),
  };

#endif
