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
 * Audio clip schema.
 */

#ifndef __SCHEMAS_AUDIO_CLIP_H__
#define __SCHEMAS_AUDIO_CLIP_H__

#include <stdbool.h>

#include "utils/types.h"
#include "utils/yaml.h"

typedef struct AudioClip_v1
{
  int        schema_version;
  char *     name;
  sample_t * frames;
  long       num_frames;
  sample_t * ch_frames[16];
  channels_t channels;
  bpm_t      bpm;
  int        samplerate;
  int        pool_id;
  long       frames_written;
  gint64     last_write;
} AudioClip_v1;

static const cyaml_schema_field_t audio_clip_fields_schema_v1[] = {
  YAML_FIELD_INT (AudioClip_v1, schema_version),
  YAML_FIELD_STRING_PTR (AudioClip_v1, name),
  YAML_FIELD_FLOAT (AudioClip_v1, bpm),
  YAML_FIELD_INT (AudioClip_v1, samplerate),
  YAML_FIELD_INT (AudioClip_v1, pool_id),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t audio_clip_schema_v1 = {
  YAML_VALUE_PTR (AudioClip_v1, audio_clip_fields_schema_v1),
};

#endif
